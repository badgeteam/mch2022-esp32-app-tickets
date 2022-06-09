#pragma once

#include "esp_event.h"

typedef struct {
    // Whether you can still buy this ticket.
    bool is_available;
    // The total number of this ticket.
    int  total;
    // the number for sale for this ticket.
    int  unsold;
    // The sold number of this ticket.
    int  sold;
} ticket_sales_t;

void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
