#include "pir_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "PIR_DRIVER";

/* Stored user callback */
static pir_callback_t s_user_callback = NULL;

/* Timestamp of last event for debounce */
static uint32_t s_last_trigger_ms = 0;

/* ─── ISR Handler ─────────────────────────────────────────────── */

static void IRAM_ATTR pir_isr_handler(void *arg)
{
    uint32_t now = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;

    /* Debounce: ignore if triggered too soon after last event */
    if ((now - s_last_trigger_ms) < PIR_DEBOUNCE_MS) {
        return;
    }

    s_last_trigger_ms = now;

    /* Call user callback if registered */
    if (s_user_callback != NULL) {
        s_user_callback();
    }
}

/* ─── Public API ──────────────────────────────────────────────── */

void pir_driver_init(pir_callback_t callback)
{
    s_user_callback = callback;

    /* Configure GPIO as input, no pull resistors (PIR has its own) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIR_GPIO_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_POSEDGE,   /* Trigger on rising edge */
    };
    gpio_config(&io_conf);

    /* Install GPIO ISR service and attach handler */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIR_GPIO_PIN, pir_isr_handler, NULL);

    ESP_LOGI(TAG, "PIR driver initialized on GPIO%d", PIR_GPIO_PIN);
}

bool pir_driver_is_active(void)
{
    return (gpio_get_level(PIR_GPIO_PIN) == 1);
}

void pir_driver_disable(void)
{
    gpio_isr_handler_remove(PIR_GPIO_PIN);
    ESP_LOGI(TAG, "PIR interrupt disabled");
}

void pir_driver_enable(void)
{
    gpio_isr_handler_add(PIR_GPIO_PIN, pir_isr_handler, NULL);
    ESP_LOGI(TAG, "PIR interrupt enabled");
}