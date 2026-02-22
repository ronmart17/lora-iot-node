/**
 * LoRa IoT Node - Transmitter
 *
 * FreeRTOS tasks:
 *   event_task  (P5) - Reads PIR events from queue, builds lora_packet_t
 *   lora_tx_task(P4) - Serializes and transmits packets over LoRa
 *   power_task  (P3) - Monitors battery and manages sleep states
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "lora_driver.h"
#include "oled_driver.h"
#include "pir_driver.h"
#include "power_driver.h"
#include "packet.h"
#include "event_service.h"
#include "lora_service.h"
#include "display_service.h"
#include "power_manager.h"

static const char *TAG = "TX_MAIN";

#define NODE_ID  0x01

/* Inter-task queue for ready packets */
static QueueHandle_t s_tx_queue = NULL;
#define TX_QUEUE_SIZE  5

/* Packet counter */
static uint32_t s_tx_count = 0;

/* ─── PIR Callback (ISR context) ─────────────────────────────── */

static void pir_motion_cb(void)
{
    event_service_push(EVENT_PIR_MOTION);
}

/* ─── Event Task (Priority 5) ────────────────────────────────── */

static void event_task(void *arg)
{
    ESP_LOGI(TAG, "Event task started");

    while (1) {
        lora_packet_t pkt;

        if (event_service_build_packet(&pkt, NODE_ID)) {
            /* Notify power manager that activity occurred */
            power_manager_notify_event();

            /* Send packet to TX queue */
            if (xQueueSend(s_tx_queue, &pkt, pdMS_TO_TICKS(500)) != pdTRUE) {
                ESP_LOGW(TAG, "TX queue full - dropping packet");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ─── LoRa TX Task (Priority 4) ──────────────────────────────── */

static void lora_tx_task(void *arg)
{
    ESP_LOGI(TAG, "LoRa TX task started");

    while (1) {
        lora_packet_t pkt;

        /* Block until a packet arrives */
        if (xQueueReceive(s_tx_queue, &pkt, pdMS_TO_TICKS(1000)) == pdTRUE) {
            bool ok = lora_service_send_packet(&pkt);

            if (ok) {
                s_tx_count++;
                display_service_show_tx(&pkt, s_tx_count);
                ESP_LOGI(TAG, "TX #%lu OK", s_tx_count);
            } else {
                ESP_LOGE(TAG, "TX FAILED");
            }
        }
    }
}

/* ─── Power Task (Priority 3) ────────────────────────────────── */

static void power_task(void *arg)
{
    ESP_LOGI(TAG, "Power task started");

    while (1) {
        power_manager_tick();

        /* Periodically send heartbeat with battery level */
        static uint32_t heartbeat_counter = 0;
        heartbeat_counter++;

        /* Heartbeat every ~60 seconds (600 * 100ms) */
        if (heartbeat_counter >= 600) {
            heartbeat_counter = 0;
            event_service_push(EVENT_HEARTBEAT);

            /* Check for low battery event */
            if (power_driver_is_low()) {
                event_service_push(EVENT_LOW_BATTERY);
                ESP_LOGW(TAG, "Low battery detected");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ─── Main ────────────────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "=== LoRa IoT Node - Transmitter ===");
    ESP_LOGI(TAG, "Node ID: 0x%02X", NODE_ID);

    /* Initialize display */
    display_service_init();
    display_service_show_boot(NODE_ID);
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Initialize LoRa */
    if (!lora_service_init()) {
        ESP_LOGE(TAG, "LoRa init FAILED - halting");
        return;
    }

    /* Initialize event service */
    event_service_init();

    /* Initialize power manager (includes ADC + battery) */
    power_manager_init();

    /* Initialize PIR sensor */
    pir_driver_init(pir_motion_cb);

    /* Create inter-task TX queue */
    s_tx_queue = xQueueCreate(TX_QUEUE_SIZE, sizeof(lora_packet_t));

    /* Show idle screen */
    uint8_t batt = power_driver_read_percent();
    display_service_show_idle(batt);

    ESP_LOGI(TAG, "All systems ready - waiting for PIR events");

    /* Launch FreeRTOS tasks */
    xTaskCreate(event_task,   "event_task",   4096, NULL, 5, NULL);
    xTaskCreate(lora_tx_task, "lora_tx_task", 4096, NULL, 4, NULL);
    xTaskCreate(power_task,   "power_task",   4096, NULL, 3, NULL);
}
