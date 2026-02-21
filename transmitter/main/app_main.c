#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "drivers/pir_driver.h"
#include "drivers/lora_driver.h"
#include "services/event_service.h"
#include "services/lora_service.h"
#include "services/power_manager.h"
#include "protocol/packet.h"

static const char *TAG = "APP_MAIN";

/* This node unique ID */
#define NODE_ID     0x01

/* TX queue - holds packets ready to transmit */
static QueueHandle_t s_queue_tx = NULL;

/* ─── PIR Callback (called from ISR) ─────────────────────────── */

static void pir_motion_callback(void)
{
    /* Push motion event to event service queue */
    event_service_push(EVENT_PIR_MOTION);

    /* Notify power manager to reset idle timer */
    power_manager_notify_event();
}

/* ─── Task: Build packets from events ────────────────────────── */

static void event_task(void *arg)
{
    lora_packet_t pkt;

    ESP_LOGI(TAG, "event_task started");

    while (1) {
        /* Try to build a packet from next event in queue */
        if (event_service_build_packet(&pkt, NODE_ID)) {

            /* Push packet to TX queue */
            if (xQueueSend(s_queue_tx, &pkt, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "TX queue full, dropping packet");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ─── Task: Transmit packets over LoRa ───────────────────────── */

static void lora_tx_task(void *arg)
{
    lora_packet_t pkt;

    ESP_LOGI(TAG, "lora_tx_task started");

    while (1) {
        /* Wait for a packet in the TX queue */
        if (xQueueReceive(s_queue_tx, &pkt, pdMS_TO_TICKS(500)) == pdTRUE) {

            ESP_LOGI(TAG, "Transmitting packet - event:0x%02X batt:%d%%",
                     pkt.event_type, pkt.battery_level);

            /* Send over LoRa */
            lora_service_send_packet(&pkt);

            /* Small delay between transmissions */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/* ─── Task: Power management ─────────────────────────────────── */

static void power_task(void *arg)
{
    ESP_LOGI(TAG, "power_task started");

    while (1) {
        /* Check battery and manage sleep states */
        power_manager_tick();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ─── Main ────────────────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "=== LoRa IoT Node - Transmitter ===");
    ESP_LOGI(TAG, "Node ID: 0x%02X", NODE_ID);

    /* Initialize all services */
    event_service_init();
    power_manager_init();

    /* Initialize LoRa module */
    if (!lora_service_init()) {
        ESP_LOGE(TAG, "Failed to initialize LoRa - halting");
        return;
    }

    /* Initialize PIR sensor with callback */
    pir_driver_init(pir_motion_callback);

    /* Create TX queue */
    s_queue_tx = xQueueCreate(5, sizeof(lora_packet_t));

    /* Create FreeRTOS tasks */
    xTaskCreate(event_task,    "event_task",    4096, NULL, 5, NULL);
    xTaskCreate(lora_tx_task,  "lora_tx_task",  4096, NULL, 4, NULL);
    xTaskCreate(power_task,    "power_task",     2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "All tasks started - system running");
}