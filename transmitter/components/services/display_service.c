#include "display_service.h"
#include "oled_driver.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "DISPLAY_TX";

void display_service_init(void)
{
    oled_driver_init();
    ESP_LOGI(TAG, "Display service initialized");
}

void display_service_show_boot(uint8_t node_id)
{
    oled_driver_clear();

    oled_driver_print(0,  0, "LoRa IoT Node",   OLED_FONT_SMALL);
    oled_driver_print(0,  8, "Transmitter",      OLED_FONT_SMALL);
    oled_driver_draw_hline(0, 127, 18);

    char buf[20];
    snprintf(buf, sizeof(buf), "Node ID : 0x%02X", node_id);
    oled_driver_print(0, 22, buf, OLED_FONT_SMALL);
    oled_driver_print(0, 32, "915 MHz  SF7",     OLED_FONT_SMALL);
    oled_driver_print(0, 42, "Initializing...",  OLED_FONT_SMALL);

    oled_driver_update();
}

void display_service_show_tx(const lora_packet_t *pkt, uint32_t tx_count)
{
    oled_driver_clear();

    oled_driver_print(0,  0, "TX NODE  915MHz", OLED_FONT_SMALL);
    oled_driver_draw_hline(0, 127, 10);

    char buf[20];

    snprintf(buf, sizeof(buf), "Event : 0x%02X", pkt->event_type);
    oled_driver_print(0, 14, buf, OLED_FONT_SMALL);

    snprintf(buf, sizeof(buf), "Time  : %lus", pkt->timestamp / 1000);
    oled_driver_print(0, 24, buf, OLED_FONT_SMALL);

    snprintf(buf, sizeof(buf), "Batt  : %d%%", pkt->battery_level);
    oled_driver_print(0, 34, buf, OLED_FONT_SMALL);

    snprintf(buf, sizeof(buf), "Sent  : %lu pkts", tx_count);
    oled_driver_print(0, 44, buf, OLED_FONT_SMALL);

    oled_driver_draw_hline(0, 127, 54);
    oled_driver_print(0, 56, "CRC16 OK", OLED_FONT_SMALL);

    oled_driver_update();
}

void display_service_show_idle(uint8_t battery)
{
    oled_driver_clear();

    oled_driver_print(0,  0, "TX NODE  915MHz", OLED_FONT_SMALL);
    oled_driver_draw_hline(0, 127, 10);
    oled_driver_print(0, 20, "Waiting for",    OLED_FONT_SMALL);
    oled_driver_print(0, 30, "PIR event...",   OLED_FONT_SMALL);

    char buf[20];
    snprintf(buf, sizeof(buf), "Battery : %d%%", battery);
    oled_driver_print(0, 48, buf, OLED_FONT_SMALL);

    oled_driver_update();
}

void display_service_show_sleep(void)
{
    oled_driver_clear();
    oled_driver_print(20, 24, "Sleeping...", OLED_FONT_SMALL);
    oled_driver_update();
}