#include "util.h"

#include <lvgl.h>

void rotate_canvas(lv_obj_t *canvas) {
    uint8_t *buf = lv_canvas_get_draw_buf(canvas)->data;
    static uint8_t buf_copy[CANVAS_BUF_SIZE];
    memcpy(buf_copy, buf, sizeof(buf_copy));
    const uint32_t stride = lv_draw_buf_width_to_stride(CANVAS_SIZE, CANVAS_COLOR_FORMAT);
    lv_draw_sw_rotate(buf_copy, buf, CANVAS_SIZE, CANVAS_SIZE, stride, stride,
                      LV_DISPLAY_ROTATION_180, CANVAS_COLOR_FORMAT);
}
