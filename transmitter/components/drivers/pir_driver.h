#ifndef PIR_DRIVER_H
#define PIR_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* PIR sensor GPIO pin (Heltec V3 - J3 header pin 18) */
#define PIR_GPIO_PIN     7

/* Minimum time between events to avoid false triggers (ms) */
#define PIR_DEBOUNCE_MS  500

/**
 * @brief Callback type called when motion is detected
 */
typedef void (*pir_callback_t)(void);

/**
 * @brief Initialize PIR sensor GPIO and attach interrupt
 * @param callback Function to call when motion is detected
 */
void pir_driver_init(pir_callback_t callback);

/**
 * @brief Check if PIR is currently detecting motion
 * @return true if motion detected
 */
bool pir_driver_is_active(void);

/**
 * @brief Disable PIR interrupt (use before deep sleep)
 */
void pir_driver_disable(void);

/**
 * @brief Re-enable PIR interrupt (use after wakeup)
 */
void pir_driver_enable(void);

#endif /* PIR_DRIVER_H */