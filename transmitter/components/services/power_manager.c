#include "power_manager.h"
#include "power_driver.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "POWER_MANAGER";

/* Timestamp of last activity in ms */
static uint32_t s_last_activity_ms = 0;

/* Boot grace period - ignore low battery for first 10s */
#define BOOT_GRACE_MS   10000
static uint32_t s_boot_time_ms = 0;

/* Detect if running on USB (no battery) */
static bool s_usb_powered = false;

void power_manager_init(void)
{
    power_driver_init();
    s_last_activity_ms = (uint32_t)(esp_timer_get_time() / 1000);
    s_boot_time_ms = s_last_activity_ms;

    /* If battery reads 0%, assume USB power */
    s_usb_powered = (power_driver_read_percent() == 0);
    if (s_usb_powered) {
        ESP_LOGI(TAG, "USB power detected - sleep disabled");
    }

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

    /* On USB power: no sleep, no battery checks */
    if (s_usb_powered) {
        return;
    }

    /* Skip during boot grace period */
    if ((now - s_boot_time_ms) < BOOT_GRACE_MS) {
        return;
    }

    /* Critical battery - enter deep sleep */
    uint8_t batt = power_driver_read_percent();
    if (batt < 5 && batt > 0) {
        ESP_LOGW(TAG, "Critical battery (%d%%)! Entering deep sleep...", batt);
        power_driver_deep_sleep(POWER_DEEP_SLEEP_MS);
        return;
    }

    /* Idle too long - enter light sleep with timer fallback */
    uint32_t idle_time = now - s_last_activity_ms;
    if (idle_time >= POWER_IDLE_TIMEOUT_MS) {
        ESP_LOGI(TAG, "Idle %lu ms - entering light sleep", idle_time);
        power_driver_light_sleep(POWER_DEEP_SLEEP_MS);  /* Wake on PIR or timer */
        power_manager_notify_event();
    }
}
