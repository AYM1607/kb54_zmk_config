#include "util.h"

#include <lvgl.h>

void rotate_canvas(lv_obj_t *canvas, lv_color_t *cbuf) {
    // 180° rotation is a simple buffer reversal
    int total = CANVAS_SIZE * CANVAS_SIZE;
    for (int i = 0; i < total / 2; i++) {
        lv_color_t tmp = cbuf[i];
        cbuf[i] = cbuf[total - 1 - i];
        cbuf[total - 1 - i] = tmp;
    }
    lv_obj_invalidate(canvas);
}
