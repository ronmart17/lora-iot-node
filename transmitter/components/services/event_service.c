#include "event_service.h"
#include "protocol/packet.h"
#include "drivers/power_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"

static const char *TAG = "EVENT_SERVICE";

/* Internal event queue */
static QueueHandle_t s_event_queue = NULL;

void event_service_init(void)
{
    s_event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(uint8_t));
    ESP_LOGI(TAG, "Event service initialized, queue size: %d", EVENT_QUEUE_SIZE);
}

void event_service_push(uint8_t event_type)
{
    if (s_event_queue == NULL) return;

    /* Non-blocking send - if queue is full, drop oldest event */
    if (xQueueSendFromISR(s_event_queue, &event_type, NULL) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue full, dropping event");
    }
}

bool event_service_build_packet(lora_packet_t *pkt, uint8_t node_id)
{
    uint8_t event_type;

    /* Wait up to 100ms for an event */
    if (xQueueReceive(s_event_queue, &event_type, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    /* Get current timestamp in ms since boot */
    uint32_t timestamp = (uint32_t)(esp_timer_get_time() / 1000);

    /* Read battery level */
    uint8_t battery = power_driver_read_percent();

    /* Build packet with CRC */
    packet_build(pkt, node_id, timestamp, event_type, battery);

    ESP_LOGI(TAG, "Packet built - node:%d event:0x%02X batt:%d%%",
             node_id, event_type, battery);

    return true;
}