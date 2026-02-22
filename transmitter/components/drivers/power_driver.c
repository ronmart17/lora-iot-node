#include "power_driver.h"
#include "pir_driver.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "POWER_DRIVER";

/* ADC handle */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

/* Heltec V3: GPIO 1 = ADC1_CHANNEL_0 */
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_0

/* ─── Init ────────────────────────────────────────────────────── */

void power_driver_init(void)
{
    /* Configure ADC unit */
    adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&adc_cfg, &s_adc_handle);

    /* Configure ADC channel for battery pin */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(s_adc_handle, BATTERY_ADC_CHANNEL, &chan_cfg);

    /* Disable WiFi and BT - not needed */
    power_driver_disable_radio();

    ESP_LOGI(TAG, "Power driver initialized (battery ADC on GPIO%d)", BATTERY_ADC_PIN);
}

/* ─── Battery ─────────────────────────────────────────────────── */

uint32_t power_driver_read_voltage(void)
{
    /* Average 16 samples to reduce ADC noise */
    uint32_t sum = 0;
    int raw = 0;
    for (int i = 0; i < 16; i++) {
        adc_oneshot_read(s_adc_handle, BATTERY_ADC_CHANNEL, &raw);
        sum += raw;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    uint32_t avg = sum / 16;

    /* Convert raw ADC (0-4095) to millivolts, voltage divider x2 */
    uint32_t voltage_mv = (avg * 3300 / 4095) * 2;

    return voltage_mv;
}

uint8_t power_driver_read_percent(void)
{
    uint32_t voltage = power_driver_read_voltage();

    if (voltage >= BATTERY_FULL_MV)  return 100;
    if (voltage <= BATTERY_EMPTY_MV) return 0;

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
    /* ESP32-S3: use ext1 wakeup (ext0 not available) */
    esp_sleep_enable_ext1_wakeup_io((1ULL << PIR_GPIO_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);

    if (sleep_ms > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)sleep_ms * 1000);
    }

    ESP_LOGI(TAG, "Entering light sleep (%lu ms max)", sleep_ms);
    esp_light_sleep_start();

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        ESP_LOGI(TAG, "Wakeup: PIR motion detected");
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Wakeup: timer expired");
    }
}

void power_driver_deep_sleep(uint32_t sleep_ms)
{
    esp_sleep_enable_ext1_wakeup_io((1ULL << PIR_GPIO_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);

    if (sleep_ms > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)sleep_ms * 1000);
    }

    ESP_LOGI(TAG, "Entering deep sleep (%lu ms max)...", sleep_ms);
    esp_deep_sleep_start();
}

/* ─── Radio ───────────────────────────────────────────────────── */

void power_driver_disable_radio(void)
{
    /* WiFi and BT not used in this project */
    ESP_LOGI(TAG, "WiFi and Bluetooth not used in this project");
}
