#include "status.h"
#include "util.h"

#include <lvgl.h>

#include <zephyr/kernel.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

lv_draw_label_dsc_t layer_label;
char layer_text[20] = {};

lv_draw_label_dsc_t profile_label_left;
char profile_text_left[10] = {};
lv_draw_label_dsc_t profile_label_right;
char profile_text_right[10] = {};

lv_draw_label_dsc_t battery_label_left;
char battery_text_left[10] = {};

static void draw(struct zmk_widget_status *widget) {
    lv_obj_t *canvas = lv_obj_get_child(zmk_widget_status_obj(widget), 0);
    lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);

    /////// LAYER
    if (widget->state.layer_label == NULL || strlen(widget->state.layer_label) > 12) {
        sprintf(layer_text, "LAYER: %i", widget->state.layer_index);
    } else {
        sprintf(layer_text, "LAYER: %s", widget->state.layer_label);
    }
    lv_canvas_draw_text(canvas, 0, 15, 128, &layer_label, layer_text);

    /////// Battery
    char *battery_symbol;
    if (widget->state.charging) {
        battery_symbol = LV_SYMBOL_CHARGE;
    } else {
        uint8_t level = widget->state.battery;
        if (level >= 95) {
            battery_symbol = LV_SYMBOL_BATTERY_FULL;
        } else if (level >= 65) {
            battery_symbol = LV_SYMBOL_BATTERY_3;
        } else if (level >= 35) {
            battery_symbol = LV_SYMBOL_BATTERY_2;
        } else if (level > 5) {
            battery_symbol = LV_SYMBOL_BATTERY_1;
        } else {
            battery_symbol = LV_SYMBOL_BATTERY_EMPTY;
        }
    }
    sprintf(battery_text_left, "%s %i%%", battery_symbol, widget->state.battery);
    lv_canvas_draw_text(canvas, 0, 46, 128, &battery_label_left, battery_text_left);

    /////// PROFILE
    profile_label_left.align = LV_TEXT_ALIGN_LEFT;
    uint8_t profile_text_padding = 10;
    switch (widget->state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        profile_text_padding = 0;
        profile_label_left.align = LV_TEXT_ALIGN_CENTER;
        sprintf(profile_text_left, "%s USB", LV_SYMBOL_USB);
        sprintf(profile_text_right, "%s", "");
        break;
    case ZMK_TRANSPORT_BLE:
        sprintf(profile_text_left, "%s %i", LV_SYMBOL_BLUETOOTH,
                widget->state.active_profile_index);
        if (widget->state.active_profile_bonded) {
            if (widget->state.active_profile_connected) {
                sprintf(profile_text_right, "%s", LV_SYMBOL_OK);
            } else {
                sprintf(profile_text_right, "%s", LV_SYMBOL_CLOSE);
            }
        } else {
            sprintf(profile_text_right, "%s", LV_SYMBOL_SETTINGS);
        }
        break;
    }
    lv_canvas_draw_text(canvas, profile_text_padding, CANVAS_SIZE - 32, 128, &profile_label_left,
                        profile_text_left);
    lv_canvas_draw_text(canvas, -profile_text_padding, CANVAS_SIZE - 32, 128, &profile_label_right,
                        profile_text_right);

    rotate_canvas(canvas, widget->cbuf);
}

//////////////////////////////// Battery ////////////////////////////////////

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw(widget);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);

    return (struct battery_status_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

//////////////////////////////// Profiles ////////////////////////////////////

static void set_output_status(struct zmk_widget_status *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index + 1;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw(widget);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

//////////////////////////////// Layers ////////////////////////////////////

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw(widget);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){.index = index, .label = zmk_keymap_layer_name(index)};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

//////////////////////////////// Initialization ////////////////////////////////////

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CANVAS_SIZE, CANVAS_SIZE);

    // Canvas.
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    // Layer.
    lv_draw_label_dsc_init(&layer_label);
    layer_label.color = lv_color_black();
    layer_label.align = LV_TEXT_ALIGN_CENTER;
    layer_label.font = &lv_font_unscii_8;

    // Battery.
    lv_draw_label_dsc_init(&battery_label_left);
    battery_label_left.color = lv_color_black();
    battery_label_left.align = LV_TEXT_ALIGN_CENTER;
    battery_label_left.font = &lv_font_montserrat_22;

    // Profile.
    lv_draw_label_dsc_init(&profile_label_left);
    profile_label_left.color = lv_color_black();
    profile_label_left.align = LV_TEXT_ALIGN_LEFT;
    profile_label_left.font = &lv_font_montserrat_22;
    lv_draw_label_dsc_init(&profile_label_right);
    profile_label_right.color = lv_color_black();
    profile_label_right.align = LV_TEXT_ALIGN_RIGHT;
    profile_label_right.font = &lv_font_montserrat_22;

    sys_slist_append(&widgets, &widget->node);
    widget_layer_status_init();
    widget_battery_status_init();
    widget_output_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }