#ifndef LORA_DRIVER_H
#define LORA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* SPI Pin definitions */
#define LORA_PIN_SCK     5
#define LORA_PIN_MOSI    27
#define LORA_PIN_MISO    19
#define LORA_PIN_CS      18
#define LORA_PIN_RST     14
#define LORA_PIN_IRQ     26

/* LoRa radio settings */
#define LORA_FREQUENCY       915E6   /* 915 MHz */
#define LORA_BANDWIDTH       125E3   /* 125 kHz */
#define LORA_SPREADING_FACTOR  7     /* SF7 - fast, short range  */
#define LORA_TX_POWER         20     /* dBm - maximum power      */
#define LORA_SYNC_WORD        0x12   /* Private network sync word */

/* Packet size in bytes */
#define LORA_PACKET_SIZE       9

/**
 * @brief Initialize LoRa module over SPI
 * @return true if module responded correctly, false on error
 */
bool lora_driver_init(void);

/**
 * @brief Transmit a raw byte buffer
 * @param data   Buffer to transmit
 * @param length Number of bytes
 * @return true if transmitted successfully
 */
bool lora_driver_send(const uint8_t *data, uint8_t length);

/**
 * @brief Check if a packet has been received
 * @return true if data is available
 */
bool lora_driver_available(void);

/**
 * @brief Read received packet into buffer
 * @param buffer  Destination buffer
 * @param length  Max bytes to read
 * @return Number of bytes read
 */
uint8_t lora_driver_receive(uint8_t *buffer, uint8_t length);

/**
 * @brief Get RSSI of last received packet (signal strength)
 * @return RSSI value in dBm
 */
int lora_driver_rssi(void);

/**
 * @brief Put LoRa module into sleep mode to save power
 */
void lora_driver_sleep(void);

/**
 * @brief Wake LoRa module from sleep and set to receive mode
 */
void lora_driver_wake(void);

#endif /* LORA_DRIVER_H */