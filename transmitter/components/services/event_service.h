#ifndef EVENT_SERVICE_H
#define EVENT_SERVICE_H

#include <stdint.h>
#include "packet.h"

/* Event queue max size */
#define EVENT_QUEUE_SIZE    10

/**
 * @brief Initialize event service and internal queue
 */
void event_service_init(void);

/**
 * @brief Push a new event to the queue (called from PIR ISR callback)
 * @param event_type Type of event (EVENT_PIR_MOTION, etc.)
 */
void event_service_push(uint8_t event_type);

/**
 * @brief Build a lora_packet_t from next event in queue
 * @param pkt      Destination packet
 * @param node_id  This node's ID
 * @return true if packet was built, false if queue was empty
 */
bool event_service_build_packet(lora_packet_t *pkt, uint8_t node_id);

#endif /* EVENT_SERVICE_H */