
#include <pax_gfx.h>
#include <math.h>
#include "piechart.h"
#include "esp_log.h"

static const char *TAG = "pie";

// Draws a pie chart, X/Y are center.
void piechart(pax_buf_t *buf, float x, float y, float radius, const piechart_slice_t *slices, size_t n_slices) {
    const pax_font_t *font = pax_get_font("saira regular");
    
    // Count up slice numbers.
    float total = 0;
    for (size_t i = 0; i < n_slices; i++) {
        total += slices[i].part;
    }
    
    if (total == 0) return;
    
    // Go to the trget spot.
    pax_push_2d(buf);
    pax_apply_2d(buf, matrix_2d_translate(x, y));
    
    // Draw slices.
    float angle = M_PI * 0.5;
    for (size_t i = 0; i < n_slices; i++) {
        // Draw slice.
        float delta = (slices[i].part / total) * (M_PI * 2.0);
        pax_draw_arc(buf, slices[i].color, 0, 0, radius, angle, angle + delta-0.001);
        
        if (slices[i].name) {
            // Draw name.
            float text_angle = angle + delta / 2;
            text_angle       = fmodf(text_angle, M_PI * 2);
            float text_x     = cosf(text_angle) * radius;
            float text_y     = -sinf(text_angle) * radius;
            pax_vec1_t dims  = pax_text_size(font, font->default_size, slices[i].name);
            if (text_x < 0) text_x -= dims.x;
            if (text_y < 0) text_y -= dims.y;
            pax_draw_text(
                buf, 0xffffffff,
                font, font->default_size,
                text_x, text_y,
                slices[i].name
            );
        }
        
        // Next
        angle += delta;
        angle  = fmodf(angle, M_PI * 2.0);
    }
    
    // Go BACK.
    pax_pop_2d(buf);
}
