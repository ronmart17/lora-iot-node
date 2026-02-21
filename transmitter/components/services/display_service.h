#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <stdint.h>
#include "protocol/packet.h"

/**
 * @brief Initialize display service
 */
void display_service_init(void);

/**
 * @brief Show startup screen with node info
 * @param node_id This node's ID
 */
void display_service_show_boot(uint8_t node_id);

/**
 * @brief Update display after packet transmitted
 * @param pkt       Packet that was sent
 * @param tx_count  Total packets transmitted so far
 */
void display_service_show_tx(const lora_packet_t *pkt, uint32_t tx_count);

/**
 * @brief Show idle screen while waiting for events
 * @param battery   Battery level 0-100%
 */
void display_service_show_idle(uint8_t battery);

/**
 * @brief Show sleep screen before entering low power mode
 */
void display_service_show_sleep(void);

#endif /* DISPLAY_SERVICE_H */