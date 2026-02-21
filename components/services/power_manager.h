#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdint.h>

/* Time to wait in idle before entering light sleep (ms) */
#define POWER_IDLE_TIMEOUT_MS    5000

/* Deep sleep duration when battery is critically low (ms) */
#define POWER_DEEP_SLEEP_MS      30000

/**
 * @brief Initialize power manager
 */
void power_manager_init(void);

/**
 * @brief Call periodically from main task to manage sleep states
 *        Enters light sleep after idle timeout
 *        Enters deep sleep if battery critically low
 */
void power_manager_tick(void);

/**
 * @brief Notify power manager that an event occurred (resets idle timer)
 */
void power_manager_notify_event(void);

#endif /* POWER_MANAGER_H */