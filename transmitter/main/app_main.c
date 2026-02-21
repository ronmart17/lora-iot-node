#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "drivers/pir_driver.h"
#include "drivers/lora_driver.h"
#include "drivers/power_driver.h"
#include "services/event_service.h"
#include "services/lora_service.h"
#include "services/power_manager.h"
#include "services/display_service.h"
#include "protocol/packet.h"

static const char *TAG = "APP_TX";

/* This node unique ID */
#define NODE_ID     0x01

/* TX queue - holds packets ready to transmit */
static QueueHandle_t s_queue_tx = NULL;

/* Packet counter */
static uint32_t s_tx_count = 0;

/* ─── PIR Callback (called from ISR) ─────────────────────────── */

static void pir_motion_callback(void)
{
    event_service_push(EVENT_PIR_MOTION);
    power_manager_notify_event();
}

/* ─── Task: Build packets from events ────────────────────────── */

static void event_task(void *arg)
{
    lora_packet_t pkt;

    ESP_LOGI(TAG, "event_task started");

    while (1) {
        if (event_service_build_packet(&pkt, NODE_ID)) {
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
        if (xQueueReceive(s_queue_tx, &pkt, pdMS_TO_TICKS(500)) == pdTRUE) {

            s_tx_count++;

            /* Update OLED with TX info */
            display_service_show_tx(&pkt, s_tx_count);

            /* Transmit over LoRa */
            lora_service_send_packet(&pkt);

            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            /* No packet - show idle screen */
            uint8_t battery = power_driver_read_percent();
            display_service_show_idle(battery);
        }
    }
}

/* ─── Task: Power management ─────────────────────────────────── */

static void power_task(void *arg)
{
    ESP_LOGI(TAG, "power_task started");

    while (1) {
        if (power_driver_is_low()) {
            display_service_show_sleep();
        }
        power_manager_tick();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ─── Main ────────────────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "=== LoRa IoT Node - Transmitter v1.0 ===");
    ESP_LOGI(TAG, "Node ID: 0x%02X", NODE_ID);

    /* Initialize display first so we can show boot screen */
    display_service_init();
    display_service_show_boot(NODE_ID);
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Initialize all services */
    event_service_init();
    power_manager_init();

    /* Initialize LoRa */
    if (!lora_service_init()) {
        ESP_LOGE(TAG, "Failed to initialize LoRa - halting");
        oled_driver_print(0, 40, "LoRa FAILED!", OLED_FONT_SMALL);
        oled_driver_update();
        return;
    }

    /* Initialize PIR sensor */
    pir_driver_init(pir_motion_callback);

    /* Create TX queue */
    s_queue_tx = xQueueCreate(5, sizeof(lora_packet_t));

    /* Show idle screen */
    display_service_show_idle(power_driver_read_percent());

    /* Create FreeRTOS tasks */
    xTaskCreate(event_task,   "event_task",   4096, NULL, 5, NULL);
    xTaskCreate(lora_tx_task, "lora_tx_task", 4096, NULL, 4, NULL);
    xTaskCreate(power_task,   "power_task",   2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "All tasks started - transmitter running");
}