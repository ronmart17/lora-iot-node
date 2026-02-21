#include "packet.h"
#include "crc16.h"
#include <string.h>

void packet_build(lora_packet_t *pkt, uint8_t node_id,
                  uint32_t timestamp, uint8_t event_type,
                  uint8_t battery_level)
{
    pkt->node_id       = node_id;
    pkt->timestamp     = timestamp;
    pkt->event_type    = event_type;
    pkt->battery_level = battery_level;

    /* Serialize payload (7 bytes) before CRC calculation */
    uint8_t buffer[PACKET_PAYLOAD_SIZE];
    buffer[0] = pkt->node_id;
    buffer[1] = (pkt->timestamp >> 24) & 0xFF;
    buffer[2] = (pkt->timestamp >> 16) & 0xFF;
    buffer[3] = (pkt->timestamp >>  8) & 0xFF;
    buffer[4] = (pkt->timestamp)       & 0xFF;
    buffer[5] = pkt->event_type;
    buffer[6] = pkt->battery_level;

    pkt->crc = crc16_calculate(buffer, PACKET_PAYLOAD_SIZE);
}

void packet_serialize(const lora_packet_t *pkt, uint8_t *buffer)
{
    buffer[0] = pkt->node_id;
    buffer[1] = (pkt->timestamp >> 24) & 0xFF;
    buffer[2] = (pkt->timestamp >> 16) & 0xFF;
    buffer[3] = (pkt->timestamp >>  8) & 0xFF;
    buffer[4] = (pkt->timestamp)       & 0xFF;
    buffer[5] = pkt->event_type;
    buffer[6] = pkt->battery_level;
    buffer[7] = (pkt->crc >> 8) & 0xFF;
    buffer[8] = (pkt->crc)      & 0xFF;
}

void packet_deserialize(const uint8_t *buffer, lora_packet_t *pkt)
{
    pkt->node_id       = buffer[0];
    pkt->timestamp     = ((uint32_t)buffer[1] << 24) |
                         ((uint32_t)buffer[2] << 16) |
                         ((uint32_t)buffer[3] <<  8) |
                         ((uint32_t)buffer[4]);
    pkt->event_type    = buffer[5];
    pkt->battery_level = buffer[6];
    pkt->crc           = ((uint16_t)buffer[7] << 8) | buffer[8];
}

bool packet_validate(const lora_packet_t *pkt)
{
    uint8_t buffer[PACKET_PAYLOAD_SIZE];
    buffer[0] = pkt->node_id;
    buffer[1] = (pkt->timestamp >> 24) & 0xFF;
    buffer[2] = (pkt->timestamp >> 16) & 0xFF;
    buffer[3] = (pkt->timestamp >>  8) & 0xFF;
    buffer[4] = (pkt->timestamp)       & 0xFF;
    buffer[5] = pkt->event_type;
    buffer[6] = pkt->battery_level;

    /* Recalculate and compare against received CRC */
    uint16_t calculated = crc16_calculate(buffer, PACKET_PAYLOAD_SIZE);
    return (calculated == pkt->crc);
}