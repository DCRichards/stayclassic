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
static TextLayer *s_pwr_label_layer;
static TextLayer *s_branding_layer;
static Layer *s_battery_layer;
static Layer *s_time_bg_layer;
static Layer *s_date_bg_layer;
static Layer *s_bt_bg_layer;
static BitmapLayer *s_bt_bitmap_layer;

static GBitmap *s_bt_connected_bitmap;
static GBitmap *s_bt_disconnected_bitmap;
static GFont digital_font_60;
static GFont digital_font_28;
static GFont digital_font_18;

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
    static char buffer[] = "Sep 31";
    strftime(buffer, sizeof(buffer), "%b %d", tick_time);
    upper_case(buffer);
    text_layer_set_text(s_date_layer, buffer);
}

static void update_battery() {
    BatteryChargeState charge_state = battery_state_service_peek();
    if (charge_state.is_charging) {
        // TODO: charging indicator
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

static void draw_time_bg(Layer *current_layer, GContext *ctx) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0,0,130,60), 3, GCornersAll);
}

static void draw_date_bg(Layer *current_layer, GContext *ctx) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0,0,80,28), 3, GCornersAll);
}

static void draw_bt_bg(Layer *current_layer, GContext *ctx) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0,0,35,28), 3, GCornersAll);
}

static void draw_battery_bar(Layer *current_layer, GContext *ctx) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 2);
    // outer box
    GRect outer_bar = GRect(0, 0, 100, 12);
    graphics_draw_rect(ctx, outer_bar);
    if (charge_level > 0) {
        // inner fill
        GRect battery_level_bar = GRect(0, 0, charge_level, 12);
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
    digital_font_60 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_60));
    digital_font_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_28));
    digital_font_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_18));
        
    // bluetooth layer
    s_bt_connected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_IMG_CON);
    s_bt_disconnected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_IMG_DISCON);
    s_bt_bitmap_layer = bitmap_layer_create(GRect(17, 28, 20, 20));
    
    // time layer
    s_time_layer = text_layer_create(GRect(0, 52, 130, 80));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_font(s_time_layer, digital_font_60);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
    
    // pwr label layer (24h)
    s_pwr_label_layer = text_layer_create(GRect(8, 126, 40, 40));
    text_layer_set_background_color(s_pwr_label_layer, GColorClear);
    text_layer_set_text_color(s_pwr_label_layer, GColorWhite);
    text_layer_set_text(s_pwr_label_layer, "PWR");
    text_layer_set_font(s_pwr_label_layer, digital_font_18);
    
    // time background
    s_time_bg_layer = layer_create(GRect(8,62,140,80));
    layer_set_update_proc(s_time_bg_layer, draw_time_bg);
    
    // date background
    s_date_bg_layer = layer_create(GRect(58,24,140,80));
    layer_set_update_proc(s_date_bg_layer, draw_date_bg);
    
    // bt background
    s_bt_bg_layer = layer_create(GRect(8,24,40,35));
    layer_set_update_proc(s_bt_bg_layer, draw_bt_bg);

    // battery layer
    s_battery_layer = layer_create(GRect(38, 132, 100, 14));
    layer_set_update_proc(s_battery_layer, draw_battery_bar);
    
    // date layer
    s_date_layer = text_layer_create(GRect(59, 19, 80, 34));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorBlack);
    text_layer_set_text(s_date_layer, "Sep 31");
    text_layer_set_font(s_date_layer, digital_font_28);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    
    // branding layer
    s_branding_layer = text_layer_create(GRect(8, 1, 150, 30));
    text_layer_set_background_color(s_branding_layer, GColorClear);
    text_layer_set_text_color(s_branding_layer, GColorWhite);
    text_layer_set_text(s_branding_layer, "|||| STAYCLASSIC");
    text_layer_set_font(s_branding_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(s_branding_layer, GTextAlignmentLeft);
    
    // assign layers - order is important here
    layer_add_child(window_get_root_layer(window), s_time_bg_layer);
    layer_add_child(window_get_root_layer(window), s_date_bg_layer);
    layer_add_child(window_get_root_layer(window), s_bt_bg_layer);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_bitmap_layer));
    layer_add_child(window_get_root_layer(window), s_battery_layer);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_pwr_label_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));    
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_branding_layer));    
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_pwr_label_layer);
    layer_destroy(s_battery_layer);
    layer_destroy(s_time_bg_layer);
    layer_destroy(s_date_bg_layer);
    layer_destroy(s_bt_bg_layer);
    
    gbitmap_destroy(s_bt_connected_bitmap);
    gbitmap_destroy(s_bt_disconnected_bitmap);
    bitmap_layer_destroy(s_bt_bitmap_layer);
    
    fonts_unload_custom_font(digital_font_60);
    fonts_unload_custom_font(digital_font_28);
    fonts_unload_custom_font(digital_font_18);
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