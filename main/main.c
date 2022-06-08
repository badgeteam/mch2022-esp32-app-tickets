
// This file contains a simple hello world app which you can base you own apps on.

#include "main.h"
#include "hardware.h"
#include "ili9341.h"

#include "pax_gfx.h"
#include "pax_codecs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_event.h"
#include "mqtt_client.h"

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
    
    // Subscrer to MQTT.
    const esp_mqtt_client_config_t init_cfg = {
        .event_loop_handle = client_events,
        .host              = "mqtt.msh2022.org",
        .port              = 1883,
    };
    client = esp_mqtt_client_init(&init_cfg);
    
    // Tset.
    const pax_font_t *font = pax_get_font("saira regular");
    pax_background(&buf, 0);
    pax_draw_text(&buf, -1, font, 24, 10, 10, "Wow!");
    disp_flush();
    
}
