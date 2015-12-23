/**
 * A first attempt at writing a Pebble watch face
 *
 * DCRichards
 * 27-07-2015
 **/

#include <pebble.h>
#include "utilities.h"

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_bt_bitmap_layer;

static GBitmap *s_bt_connected_bitmap;
static GBitmap *s_bt_disconnected_bitmap;
static GFont digital_font_72;
static GFont digital_font_28;

static const uint32_t const disconnect_vibrate_pattern[] = { 100, 100, 100 };
static const uint32_t const connect_vibrate_pattern[] = { 100 };

static int charge_level;

static void update_time() {
    time_t temp = time(NULL); 
    struct tm *tick_time = localtime(&temp);
    static char buffer[] = "00:00";
    
    if (clock_is_24h_style() == true) {
        strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
        strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    }

    text_layer_set_text(s_time_layer, buffer);
}

static void update_date() {
    time_t temp = time(NULL); 
    struct tm *tick_time = localtime(&temp);
    static char buffer[] = "Sept 31";
    strftime(buffer, sizeof(buffer), "%b %d", tick_time);
    upper_case(buffer);
    text_layer_set_text(s_date_layer, buffer);
}

static void update_battery() {
    static char s_battery_buffer[16];
    BatteryChargeState charge_state = battery_state_service_peek();
    if (charge_state.is_charging) {
        
    } else {
        layer_mark_dirty(s_battery_layer);
        charge_level = charge_state.charge_percent;
    }
}

static void update_bt_status() {
    if (bluetooth_connection_service_peek()) {
        bitmap_layer_set_bitmap(s_bt_bitmap_layer, s_bt_connected_bitmap);
    } else {
        bitmap_layer_set_bitmap(s_bt_bitmap_layer, s_bt_disconnected_bitmap);
    }
}

static void draw_battery_bar(Layer *current_layer, GContext *ctx) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 2);
    if (charge_level > 0) {
        graphics_draw_rect(ctx, GRect(0, 0, 100, 8));
        GRect battery_level_bar = GRect(0, 0, charge_level, 8);
        graphics_draw_rect(ctx, battery_level_bar);
        graphics_fill_rect(ctx, battery_level_bar, 0, GCornerNone);
    }
    
}

static void battery_handler(BatteryChargeState charge_state) {
    update_battery();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
    update_date();
}

static void bt_handler(bool connected) {
    // vibrate here otherwise we will vibe every time the watch face loads.
    if (connected) {
        VibePattern pat = {
            .durations = connect_vibrate_pattern,
            .num_segments = ARRAY_LENGTH(connect_vibrate_pattern),
        };
        vibes_enqueue_custom_pattern(pat);    
    } else {
        VibePattern pat = {
            .durations = disconnect_vibrate_pattern,
            .num_segments = ARRAY_LENGTH(disconnect_vibrate_pattern),
        };
        vibes_enqueue_custom_pattern(pat);
    }
    update_bt_status();
}

static void main_window_load(Window *window) {
    digital_font_72 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_72));
    digital_font_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_28));
        
    // bluetooth layer
    s_bt_connected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_IMG_CON);
    s_bt_disconnected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_IMG_DISCON);
    s_bt_bitmap_layer = bitmap_layer_create(GRect(20, 30, 20, 20));
    
    // time layer
    s_time_layer = text_layer_create(GRect(0, 40, 144, 80));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_font(s_time_layer, digital_font_72);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    
    // battery layer
    s_battery_layer = layer_create(GRect(20, 150, 100, 10));
    
    // date layer
    s_date_layer = text_layer_create(GRect(48, 20, 80, 34));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_text(s_date_layer, "Sept 31");
    text_layer_set_font(s_date_layer, digital_font_28);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    
    // assign layers - order is important here
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_bitmap_layer));
    layer_add_child(window_get_root_layer(window), s_battery_layer);
    layer_set_update_proc(s_battery_layer, draw_battery_bar);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));   
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    layer_destroy(s_battery_layer);
    
    gbitmap_destroy(s_bt_connected_bitmap);
    gbitmap_destroy(s_bt_disconnected_bitmap);
    bitmap_layer_destroy(s_bt_bitmap_layer);
    
    fonts_unload_custom_font(digital_font_72);
    fonts_unload_custom_font(digital_font_28);
}

static void init() {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_handler);
    bluetooth_connection_service_subscribe(bt_handler);
    
    s_main_window = window_create();
    
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    
    window_set_background_color(s_main_window, GColorBlack);
    window_stack_push(s_main_window, true);
    
    // we need to update at the start
    update_time();
    update_date();
    update_battery();
    update_bt_status();
}

static void deinit() {
    tick_timer_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}