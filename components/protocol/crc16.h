#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>

/**
 * @brief Calculate CRC16-CCITT of a data buffer
 * @param data   Pointer to the buffer
 * @param length Number of bytes
 * @return Calculated CRC16
 */
uint16_t crc16_calculate(const uint8_t *data, uint16_t length);

#endif /* CRC16_H */