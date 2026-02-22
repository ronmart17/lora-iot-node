#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "lora_driver.h"
#include "lora_service.h"
#include "display_service.h"
#include "packet.h"
#include "oled_driver.h"

static const char *TAG = "APP_RX";

/* RX queue - holds received packets */
static QueueHandle_t s_queue_rx = NULL;

/* Counters */
static uint32_t s_rx_count    = 0;
static uint32_t s_error_count = 0;

/* ─── Task: Receive LoRa packets ──────────────────────────────── */

static void lora_rx_task(void *arg)
{
    lora_packet_t pkt;

    ESP_LOGI(TAG, "lora_rx_task started");

    while (1) {
        if (lora_service_receive_packet(&pkt)) {

            s_rx_count++;

            /* Push valid packet to display queue */
            if (xQueueSend(s_queue_rx, &pkt, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "RX queue full, dropping packet");
            }

        } else if (lora_driver_available()) {
            /* Packet available but CRC failed */
            s_error_count++;
            display_service_show_crc_error(s_error_count);
            ESP_LOGE(TAG, "CRC error #%lu", s_error_count);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ─── Task: Update display with received packet ───────────────── */

static void display_task(void *arg)
{
    lora_packet_t pkt;

    ESP_LOGI(TAG, "display_task started");

    while (1) {
        if (xQueueReceive(s_queue_rx, &pkt, pdMS_TO_TICKS(500)) == pdTRUE) {

            int rssi = lora_service_get_rssi();

            /* Update OLED with packet info */
            display_service_show_rx(&pkt, s_rx_count, rssi);

            ESP_LOGI(TAG, "Displayed packet #%lu - node:0x%02X event:0x%02X rssi:%d",
                     s_rx_count, pkt.node_id, pkt.event_type, rssi);

        } else {
            /* No packet received - show listening screen */
            display_service_show_listening();
        }
    }
}

/* ─── Main ────────────────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "=== LoRa IoT Node - Receiver v1.0 ===");

    /* Initialize display first */
    display_service_init();
    display_service_show_boot();
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Initialize LoRa in RX mode */
    if (!lora_service_init()) {
        ESP_LOGE(TAG, "Failed to initialize LoRa - halting");
        oled_driver_print(0, 40, "LoRa FAILED!", OLED_FONT_SMALL);
        oled_driver_update();
        return;
    }

    /* Create RX queue */
    s_queue_rx = xQueueCreate(10, sizeof(lora_packet_t));

    /* Show listening screen */
    display_service_show_listening();

    /* Create FreeRTOS tasks */
    xTaskCreate(lora_rx_task,  "lora_rx_task",  4096, NULL, 5, NULL);
    xTaskCreate(display_task,  "display_task",   4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "All tasks started - receiver running");
}