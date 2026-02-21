#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "crc16.h"

/* Event types */
#define EVENT_PIR_MOTION    0x01
#define EVENT_HEARTBEAT     0x02
#define EVENT_LOW_BATTERY   0x03

/* Payload size without CRC */
#define PACKET_PAYLOAD_SIZE  7

/**
 * @brief LoRa packet structure (9 bytes total)
 *
 *  | node_id | timestamp (4B) | event_type | battery | crc16 (2B) |
 */
typedef struct {
    uint8_t  node_id;        /* Unique transmitter node ID    */
    uint32_t timestamp;      /* Relative epoch since boot (ms)*/
    uint8_t  event_type;     /* Event type (see defines above)*/
    uint8_t  battery_level;  /* Battery level 0-100 %         */
    uint16_t crc;            /* CRC16-CCITT of payload        */
} lora_packet_t;

/**
 * @brief Build a packet and calculate its CRC
 */
void packet_build(lora_packet_t *pkt, uint8_t node_id,
                  uint32_t timestamp, uint8_t event_type,
                  uint8_t battery_level);

/**
 * @brief Serialize packet to bytes for LoRa transmission
 * @param pkt    Source packet
 * @param buffer Destination buffer (minimum 9 bytes)
 */
void packet_serialize(const lora_packet_t *pkt, uint8_t *buffer);

/**
 * @brief Deserialize received bytes into lora_packet_t structure
 * @param buffer Received bytes
 * @param pkt    Destination structure
 */
void packet_deserialize(const uint8_t *buffer, lora_packet_t *pkt);

/**
 * @brief Validate the CRC of a received packet
 * @return true if CRC is valid, false if corrupted
 */
bool packet_validate(const lora_packet_t *pkt);

#endif /* PACKET_H */