#include "power_driver.h"
#include "pir_driver.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "POWER_DRIVER";

/* ─── Init ────────────────────────────────────────────────────── */

void power_driver_init(void)
{
    /* Configure ADC for battery voltage reading */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); /* GPIO35 */

    /* Disable WiFi and BT immediately - not needed in this project */
    power_driver_disable_radio();

    ESP_LOGI(TAG, "Power driver initialized");
}

/* ─── Battery ─────────────────────────────────────────────────── */

uint32_t power_driver_read_voltage(void)
{
    /* Average 16 samples to reduce ADC noise */
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += adc1_get_raw(ADC1_CHANNEL_7);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    uint32_t raw = sum / 16;

    /* Convert raw ADC (0-4095) to millivolts
     * ESP32 ADC ref = 3300mV, voltage divider factor = 2 */
    uint32_t voltage_mv = (raw * 3300 / 4095) * 2;

    return voltage_mv;
}

uint8_t power_driver_read_percent(void)
{
    uint32_t voltage = power_driver_read_voltage();

    /* Clamp to valid range */
    if (voltage >= BATTERY_FULL_MV)  return 100;
    if (voltage <= BATTERY_EMPTY_MV) return 0;

    /* Linear interpolation between empty and full */
    uint8_t percent = (uint8_t)((voltage - BATTERY_EMPTY_MV) * 100
                                / (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
    return percent;
}

bool power_driver_is_low(void)
{
    return (power_driver_read_percent() <= BATTERY_LOW_PCT);
}

/* ─── Sleep Modes ─────────────────────────────────────────────── */

void power_driver_light_sleep(uint32_t sleep_ms)
{
    /* Configure PIR GPIO as wakeup source */
    esp_sleep_enable_ext0_wakeup(PIR_GPIO_PIN, 1);  /* Wake on HIGH */

    /* Optional timer wakeup */
    if (sleep_ms > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)sleep_ms * 1000);
    }

    ESP_LOGI(TAG, "Entering light sleep (%lu ms max)", sleep_ms);

    /* Light sleep - CPU halts but RAM is retained */
    esp_light_sleep_start();

    /* Execution resumes here after wakeup */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGI(TAG, "Wakeup: PIR motion detected");
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Wakeup: timer expired");
    }
}

void power_driver_deep_sleep(uint32_t sleep_ms)
{
    /* Configure PIR GPIO as external wakeup source */
    esp_sleep_enable_ext0_wakeup(PIR_GPIO_PIN, 1);

    /* Optional timer wakeup */
    if (sleep_ms > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)sleep_ms * 1000);
    }

    ESP_LOGI(TAG, "Entering deep sleep (%lu ms max)...", sleep_ms);

    /* Deep sleep - CPU resets on wakeup, RAM lost */
    esp_deep_sleep_start();

    /* Code never reaches here after deep sleep */
}

/* ─── Radio ───────────────────────────────────────────────────── */

void power_driver_disable_radio(void)
{
    /* Disable WiFi */
    esp_wifi_stop();
    esp_wifi_deinit();

    /* Disable Bluetooth */
    esp_bt_controller_disable();

    ESP_LOGI(TAG, "WiFi and Bluetooth disabled");
}