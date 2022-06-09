
// This file contains a simple hello world app which you can base you own apps on.

#include "main.h"
#include "hardware.h"
#include "ili9341.h"

#include "pax_gfx.h"
#include "pax_codecs.h"
#include "piechart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "wifi_connect.h"
#include "wifi_connection.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "cJSON.h"

/* Topics:
mch2022/ticketshop/RegularTickets
mch2022/ticketshop/CamperTicket
mch2022/ticketshop/Early-BirdTickets
mch2022/ticketshop/Harbourover12m
mch2022/ticketshop/Harbourunder12m
*/

ticket_sales_t regularTickets;
ticket_sales_t earlyBirdTickets;
ticket_sales_t camperTickets;
ticket_sales_t harbourOver12m;
ticket_sales_t harbourUnder12m;

static pax_buf_t buf;
xQueueHandle buttonQueue;

static const char *TAG = "mch2022-demo-app";

void disp_flush() {
    ili9341_write(get_ili9341(), buf.buf);
}

esp_mqtt_client_handle_t client;
esp_event_loop_handle_t  client_events;

void app_main() {
    // Init HW.
    bsp_init();
    bsp_rp2040_init();
    buttonQueue = get_rp2040()->queue;
    
    // Init GFX.
    pax_buf_init(&buf, NULL, 320, 240, PAX_BUF_16_565RGB);
    
    // Create event loop.
    const esp_event_loop_args_t event_args = {
        .queue_size      = 32,
        .task_core_id    = 1,
        .task_name       = "mqtt-events",
        .task_priority   = 1,
        .task_stack_size = 2048,
    };
    esp_event_loop_create(&event_args, &client_events);
    
    // Init NVS and ask for WiFi.
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    wifi_connect_to_stored();
    
    // Subscrer to MQTT.
    const esp_mqtt_client_config_t init_cfg = {
        .event_loop_handle = client_events,
        .uri               = "mqtt://mqtt.mch2022.org:1883/",
    };
    client = esp_mqtt_client_init(&init_cfg);
    
    // Add MQTT event handlers.
    // esp_event_handler_instance_register_with(client_events, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    
    // Start MQTT.
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    
    while (1) {
        // Main ticket pie chart.
        pax_background(&buf, 0);
        int unsold = regularTickets.total - regularTickets.sold;
        char buf0[64];
        char buf1[64];
        char buf2[64];
        const piechart_slice_t slices[] = {
            { .name = buf0, .part = earlyBirdTickets.total, .color = 0xffff7f00, },
            { .name = buf1, .part = regularTickets.sold,    .color = 0xff00ff3f, },
            { .name = buf2, .part = unsold,                 .color = 0xff007fff, },
        };
        
        // Make some names.
        snprintf(buf0, 64, "%d",   (int)slices[0].part);
        snprintf(buf1, 64, "%d",      (int)slices[1].part);
        snprintf(buf2, 64, "%d", (int)slices[2].part);
        
        // Draw the pie chart.
        const size_t n_slices = sizeof(slices) / sizeof(piechart_slice_t);
        piechart(&buf, buf.width-buf.height/2, buf.height/2, 100, slices, n_slices);
        
        // Draw the legend.
        const pax_font_t *font = pax_get_font("saira regular");
        pax_draw_circle(&buf, 0xffff7f00, 5, buf.height/2-20, 4);
        pax_draw_circle(&buf, 0xff00ff3f, 5, buf.height/2,    4);
        pax_draw_circle(&buf, 0xff007fff, 5, buf.height/2+20, 4);
        pax_draw_text(&buf, -1, font, 18, 10, buf.height/2-25, "Early bird");
        pax_draw_text(&buf, -1, font, 18, 10, buf.height/2- 5, "Regular");
        pax_draw_text(&buf, -1, font, 18, 10, buf.height/2+15, "Available");
        
        disp_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void mqtt_message_handler(char *topic, char *data) {
    // Log the data.
    ESP_LOGI(topic, "%s", data);
    
    cJSON *object = cJSON_Parse(data);
    if (!object) return;
    
    // Get data objects.
    cJSON *avl_obj      = cJSON_GetObjectItem(object, "available");
    cJSON *sold_num_obj = cJSON_GetObjectItem(object, "sold");
    cJSON *quota_obj    = cJSON_GetObjectItem(object, "quota");
    
    // Enforce data types.
    if (!cJSON_IsBool  (avl_obj))      goto end;
    if (!cJSON_IsNumber(sold_num_obj)) goto end;
    if (!cJSON_IsNumber(quota_obj))    goto end;
    
    // Get data.
    bool avl      = cJSON_IsTrue(avl_obj);
    int  sold_num = cJSON_GetNumberValue(sold_num_obj);
    int  quota    = cJSON_GetNumberValue(quota_obj);
    int  avl_num  = quota - sold_num;
    
    // Log the ticket.
    if (avl) {
        ESP_LOGI(topic, "Available: yes (%d), Sold: %d / %d", avl_num, sold_num, quota);
    } else {
        ESP_LOGI(topic, "Available: no, Sold: %d / %d", sold_num, quota);
    }
    
    // Update the numbers.
    ticket_sales_t sales = {
        .is_available = avl,
        .sold         = sold_num,
        .total        = quota,
    };
    if (!strcmp(topic, "mch2022/ticketshop/RegularTickets"))    regularTickets   = sales;
    if (!strcmp(topic, "mch2022/ticketshop/Early-BirdTickets")) earlyBirdTickets = sales;
    if (!strcmp(topic, "mch2022/ticketshop/CamperTicket"))      camperTickets    = sales;
    if (!strcmp(topic, "mch2022/ticketshop/Harbourover12m"))    harbourOver12m   = sales;
    if (!strcmp(topic, "mch2022/ticketshop/Harbourunder12m"))   harbourUnder12m  = sales;
    
    end:
    cJSON_Delete(object);
}

void mqtt_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    // The funny.
    esp_mqtt_event_t *event = event_data;
    if (event_id == MQTT_EVENT_CONNECTED) {
        // Connected, now subscribe.
        ESP_LOGI(TAG, "MQTT Connected.");
        esp_mqtt_client_subscribe(client, "mch2022/ticketshop/RegularTickets", 0);
        esp_mqtt_client_subscribe(client, "mch2022/ticketshop/Early-BirdTickets", 0);
        esp_mqtt_client_subscribe(client, "mch2022/ticketshop/CamperTicket", 0);
        esp_mqtt_client_subscribe(client, "mch2022/ticketshop/Harbourover12m", 0);
        esp_mqtt_client_subscribe(client, "mch2022/ticketshop/Harbourunder12m", 0);
    } else if (event_id == MQTT_EVENT_DATA) {
        // Incoming message.
        bool  data_term  = event->data [event->data_len  - 1] == 0;
        bool  topic_term = event->topic[event->topic_len - 1] == 0;
        char *data_buf   = event->data;
        char *topic_buf  = event->data;
        
        // Enforce null termination.
        if (!data_term) {
            // Make a terminated copy.
            data_buf = malloc(event->data_len + 1);
            memcpy(data_buf, event->data, event->data_len);
            data_buf[event->data_len] = 0;
        }
        if (!topic_term) {
            // Make a terminated copy.
            topic_buf = malloc(event->topic_len + 1);
            memcpy(topic_buf, event->topic, event->topic_len);
            topic_buf[event->topic_len] = 0;
        }
        
        // Handle incoming messages.
        mqtt_message_handler(topic_buf, data_buf);
        
        // Clean up.
        if (!data_term)  free(data_buf);
        if (!topic_term) free(topic_buf);
    }
}
