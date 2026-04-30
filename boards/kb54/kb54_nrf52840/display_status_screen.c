#include "widgets/status.h"

#if IS_ENABLED(CONFIG_KB54_WIDGET_STATUS)
static struct zmk_widget_status status_widget;
#endif

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;
    // Pass NULL as parent to create a new screen.
    screen = lv_obj_create(NULL);

#if IS_ENABLED(CONFIG_KB54_WIDGET_STATUS)
    zmk_widget_status_init(&status_widget, screen);
    lv_obj_align(zmk_widget_status_obj(&status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    return screen;
}