#include "lora_service.h"
#include "lora_driver.h"
#include "packet.h"
#include "esp_log.h"

static const char *TAG = "LORA_SERVICE_RX";

bool lora_service_init(void)
{
    bool ok = lora_driver_init();
    if (!ok) {
        ESP_LOGE(TAG, "LoRa service failed to initialize");
        return false;
    }

    /* Set to continuous receive mode */
    lora_driver_wake();

    ESP_LOGI(TAG, "LoRa service ready in RX mode");
    return true;
}

bool lora_service_receive_packet(lora_packet_t *pkt)
{
    if (!lora_driver_available()) {
        return false;
    }

    uint8_t buffer[LORA_PACKET_SIZE];
    uint8_t received = lora_driver_receive(buffer, LORA_PACKET_SIZE);

    if (received != LORA_PACKET_SIZE) {
        ESP_LOGW(TAG, "Unexpected packet size: %d bytes", received);
        return false;
    }

    /* Deserialize bytes into struct */
    packet_deserialize(buffer, pkt);

    /* Validate CRC */
    if (!packet_validate(pkt)) {
        ESP_LOGE(TAG, "CRC validation failed - packet corrupted");
        return false;
    }

    ESP_LOGI(TAG, "Valid packet - node:0x%02X event:0x%02X batt:%d%% rssi:%d dBm",
             pkt->node_id, pkt->event_type,
             pkt->battery_level, lora_driver_rssi());

    return true;
}

int lora_service_get_rssi(void)
{
    return lora_driver_rssi();
}