#ifndef POWER_DRIVER_H
#define POWER_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* Battery ADC pin (Heltec V3) */
#define BATTERY_ADC_PIN     1

/* Battery voltage thresholds */
#define BATTERY_FULL_MV     4200
#define BATTERY_EMPTY_MV    3300
#define BATTERY_LOW_PCT     20

/**
 * @brief Initialize power management and ADC for battery monitoring
 */
void power_driver_init(void);

/**
 * @brief Read battery voltage in millivolts
 * @return Battery voltage in mV
 */
uint32_t power_driver_read_voltage(void);

/**
 * @brief Get battery level as percentage 0-100
 * @return Battery percentage
 */
uint8_t power_driver_read_percent(void);

/**
 * @brief Check if battery is low (below BATTERY_LOW_PCT)
 * @return true if battery is low
 */
bool power_driver_is_low(void);

/**
 * @brief Enter light sleep - wakes on PIR interrupt or timer
 * @param sleep_ms Time to sleep in milliseconds (0 = until PIR wakeup)
 */
void power_driver_light_sleep(uint32_t sleep_ms);

/**
 * @brief Enter deep sleep - wakes on PIR external interrupt
 * @param sleep_ms Time to sleep in milliseconds (0 = until PIR wakeup)
 */
void power_driver_deep_sleep(uint32_t sleep_ms);

/**
 * @brief Disable WiFi and Bluetooth to save power
 */
void power_driver_disable_radio(void);

#endif /* POWER_DRIVER_H */