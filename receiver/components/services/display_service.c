#include "display_service.h"
#include "drivers/oled_driver.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "DISPLAY_RX";

void display_service_init(void)
{
    oled_driver_init();
    ESP_LOGI(TAG, "Display service initialized");
}

void display_service_show_boot(void)
{
    oled_driver_clear();

    oled_driver_print(0,  0, "LoRa IoT Node",  OLED_FONT_SMALL);
    oled_driver_print(0,  8, "Receiver",        OLED_FONT_SMALL);
    oled_driver_draw_hline(0, 127, 18);
    oled_driver_print(0, 22, "915 MHz  SF7",    OLED_FONT_SMALL);
    oled_driver_print(0, 32, "CRC16 enabled",   OLED_FONT_SMALL);
    oled_driver_print(0, 42, "Initializing...", OLED_FONT_SMALL);

    oled_driver_update();
}

void display_service_show_rx(const lora_packet_t *pkt, uint32_t rx_count, int rssi)
{
    oled_driver_clear();

    oled_driver_print(0,  0, "RX NODE  915MHz", OLED_FONT_SMALL);
    oled_driver_draw_hline(0, 127, 10);

    char buf[20];

    snprintf(buf, sizeof(buf), "Node  : 0x%02X", pkt->node_id);
    oled_driver_print(0, 14, buf, OLED_FONT_SMALL);

    snprintf(buf, sizeof(buf), "Event : 0x%02X", pkt->event_type);
    oled_driver_print(0, 24, buf, OLED_FONT_SMALL);

    snprintf(buf, sizeof(buf), "Batt  : %d%%", pkt->battery_level);
    oled_driver_print(0, 34, buf, OLED_FONT_SMALL);

    snprintf(buf, sizeof(buf), "RSSI  : %d dBm", rssi);
    oled_driver_print(0, 44, buf, OLED_FONT_SMALL);

    snprintf(buf, sizeof(buf), "Pkts  : %lu", rx_count);
    oled_driver_print(0, 54, buf, OLED_FONT_SMALL);

    oled_driver_update();
}

void display_service_show_crc_error(uint32_t error_count)
{
    oled_driver_clear();

    oled_driver_print(0,  0, "RX NODE  915MHz", OLED_FONT_SMALL);
    oled_driver_draw_hline(0, 127, 10);
    oled_driver_print(20, 22, "CRC ERROR!",     OLED_FONT_SMALL);
    oled_driver_print(10, 32, "Packet dropped", OLED_FONT_SMALL);

    char buf[20];
    snprintf(buf, sizeof(buf), "Errors: %lu", error_count);
    oled_driver_print(0, 48, buf, OLED_FONT_SMALL);

    oled_driver_update();
}

void display_service_show_listening(void)
{
    oled_driver_clear();

    oled_driver_print(0,  0, "RX NODE  915MHz", OLED_FONT_SMALL);
    oled_driver_draw_hline(0, 127, 10);
    oled_driver_print(10, 24, "Listening...",   OLED_FONT_SMALL);
    oled_driver_print(0,  34, "Waiting for",    OLED_FONT_SMALL);
    oled_driver_print(0,  44, "LoRa packets",   OLED_FONT_SMALL);

    oled_driver_update();
}