#ifndef LORA_SERVICE_H
#define LORA_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol/packet.h"

/**
 * @brief Initialize LoRa service (wraps lora_driver_init)
 * @return true if LoRa module responded correctly
 */
bool lora_service_init(void);

/**
 * @brief Serialize and transmit a lora_packet_t
 * @param pkt Packet to transmit
 * @return true if transmitted successfully
 */
bool lora_service_send_packet(const lora_packet_t *pkt);

/**
 * @brief Check for incoming packet and deserialize it
 * @param pkt Destination packet
 * @return true if valid packet received and CRC passed
 */
bool lora_service_receive_packet(lora_packet_t *pkt);

#endif /* LORA_SERVICE_H */