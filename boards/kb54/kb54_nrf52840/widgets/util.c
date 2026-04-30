#include "util.h"

#include <lvgl.h>

void rotate_canvas(lv_obj_t *canvas, lv_color_t *cbuf) {
    static lv_color_t cbuf_tmp[CANVAS_SIZE * CANVAS_SIZE];
    memcpy(cbuf_tmp, cbuf, sizeof(cbuf_tmp));
    lv_img_dsc_t img;
    img.data = (void *)cbuf_tmp;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;
    img.header.w = CANVAS_SIZE;
    img.header.h = CANVAS_SIZE;
    lv_canvas_transform(canvas, &img, 1800, LV_IMG_ZOOM_NONE, 0, 0, CANVAS_SIZE / 2,
                        CANVAS_SIZE / 2, false);
}