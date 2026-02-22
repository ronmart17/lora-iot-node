#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <stdint.h>
#include "packet.h"

/**
 * @brief Initialize display service
 */
void display_service_init(void);

/**
 * @brief Show startup screen
 */
void display_service_show_boot(void);

/**
 * @brief Show received packet info on OLED
 * @param pkt       Received packet
 * @param rx_count  Total packets received so far
 * @param rssi      Signal strength in dBm
 */
void display_service_show_rx(const lora_packet_t *pkt, uint32_t rx_count, int rssi);

/**
 * @brief Show CRC error on display
 * @param error_count Total CRC errors so far
 */
void display_service_show_crc_error(uint32_t error_count);

/**
 * @brief Show idle/listening screen
 */
void display_service_show_listening(void);

#endif /* DISPLAY_SERVICE_H */