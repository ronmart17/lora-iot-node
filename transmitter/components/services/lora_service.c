#include "lora_service.h"
#include "drivers/lora_driver.h"
#include "protocol/packet.h"
#include "esp_log.h"

static const char *TAG = "LORA_SERVICE";

bool lora_service_init(void)
{
    bool ok = lora_driver_init();
    if (ok) {
        ESP_LOGI(TAG, "LoRa service ready");
    } else {
        ESP_LOGE(TAG, "LoRa service failed to initialize");
    }
    return ok;
}

bool lora_service_send_packet(const lora_packet_t *pkt)
{
    uint8_t buffer[LORA_PACKET_SIZE];

    /* Serialize struct to raw bytes */
    packet_serialize(pkt, buffer);

    /* Transmit over LoRa */
    bool ok = lora_driver_send(buffer, LORA_PACKET_SIZE);

    if (ok) {
        ESP_LOGI(TAG, "Packet transmitted - node:%d event:0x%02X",
                 pkt->node_id, pkt->event_type);
    }

    return ok;
}

bool lora_service_receive_packet(lora_packet_t *pkt)
{
    if (!lora_driver_available()) {
        return false;
    }

    uint8_t buffer[LORA_PACKET_SIZE];
    uint8_t received = lora_driver_receive(buffer, LORA_PACKET_SIZE);

    if (received != LORA_PACKET_SIZE) {
        ESP_LOGW(TAG, "Unexpected packet size: %d", received);
        return false;
    }

    /* Deserialize bytes into struct */
    packet_deserialize(buffer, pkt);

    /* Validate CRC */
    if (!packet_validate(pkt)) {
        ESP_LOGE(TAG, "CRC validation failed - packet corrupted");
        return false;
    }

    ESP_LOGI(TAG, "Valid packet - node:%d event:0x%02X batt:%d%% rssi:%d dBm",
             pkt->node_id, pkt->event_type, pkt->battery_level,
             lora_driver_rssi());

    return true;
}