#ifndef LORA_SERVICE_H
#define LORA_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "packet.h"

/**
 * @brief Initialize LoRa service in continuous RX mode
 * @return true if LoRa module responded correctly
 */
bool lora_service_init(void);

/**
 * @brief Check for incoming packet and deserialize it
 * @param pkt  Destination packet
 * @return true if valid packet received and CRC passed
 */
bool lora_service_receive_packet(lora_packet_t *pkt);

/**
 * @brief Get RSSI of last received packet
 * @return RSSI in dBm
 */
int lora_service_get_rssi(void);

#endif /* LORA_SERVICE_H */