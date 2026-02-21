#include "power_manager.h"
#include "drivers/power_driver.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "POWER_MANAGER";

/* Timestamp of last activity in ms */
static uint32_t s_last_activity_ms = 0;

void power_manager_init(void)
{
    power_driver_init();
    s_last_activity_ms = (uint32_t)(esp_timer_get_time() / 1000);
    ESP_LOGI(TAG, "Power manager initialized");
}

void power_manager_notify_event(void)
{
    /* Reset idle timer on any activity */
    s_last_activity_ms = (uint32_t)(esp_timer_get_time() / 1000);
}

void power_manager_tick(void)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t idle_time = now - s_last_activity_ms;

    /* Critical battery - enter deep sleep */
    if (power_driver_read_percent() < 5) {
        ESP_LOGW(TAG, "Critical battery! Entering deep sleep...");
        power_driver_deep_sleep(POWER_DEEP_SLEEP_MS);
        return;
    }

    /* Idle too long - enter light sleep */
    if (idle_time >= POWER_IDLE_TIMEOUT_MS) {
        ESP_LOGI(TAG, "Idle %lu ms - entering light sleep", idle_time);
        power_driver_light_sleep(0);  /* Wake on PIR */
        power_manager_notify_event(); /* Reset timer after wakeup */
    }
}