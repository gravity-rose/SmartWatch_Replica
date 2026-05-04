#include <pebble.h>
#include "squircle_data.h"

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS  1

#define SCREEN_W 200
#define SCREEN_H 228

#define WEATHER_SUNNY   0
#define WEATHER_CLOUDY  1
#define WEATHER_RAINY   2
#define WEATHER_SNOWY   3
#define WEATHER_THUNDER 4

static Window *s_window;
static Layer *s_canvas_layer;

static TextLayer *s_title_layer;
static TextLayer *s_temp_layer;
static TextLayer *s_day_layer;
static TextLayer *s_month_layer;
static TextLayer *s_date_num_layer;
static TextLayer *s_bpm_label_layer;
static TextLayer *s_bpm_value_layer;
static TextLayer *s_hours_layer;
static TextLayer *s_minutes_layer;
static TextLayer *s_ampm_layer;
static TextLayer *s_colon_layer;
static TextLayer *s_steps_label_layer;
static TextLayer *s_steps_value_layer;
static TextLayer *s_kcal_label_layer;
static TextLayer *s_kcal_value_layer;
static TextLayer *s_battery_layer;

static BitmapLayer *s_weather_icon_layer;
static BitmapLayer *s_heart_icon_layer;
static BitmapLayer *s_steps_icon_layer;
static BitmapLayer *s_calories_icon_layer;

static GBitmap *s_weather_sunny_bmp;
static GBitmap *s_weather_cloudy_bmp;
static GBitmap *s_weather_rainy_bmp;
static GBitmap *s_weather_snowy_bmp;
static GBitmap *s_weather_thunder_bmp;
static GBitmap *s_heart_bmp;
static GBitmap *s_steps_bmp;
static GBitmap *s_calories_bmp;

static char s_temp_buf[16];
static char s_day_buf[8];
static char s_month_buf[8];
static char s_date_num_buf[4];
static char s_hours_buf[4];
static char s_minutes_buf[4];
static char s_ampm_buf[4];
static char s_bpm_buf[12];
static char s_steps_buf[12];
static char s_kcal_buf[12];
static char s_battery_buf[12];

static int s_weather_condition = WEATHER_SUNNY;
static bool s_bt_connected = true;
static bool s_battery_charging = false;
static int s_battery_level = 0;

static GFont s_font_time;
static GFont s_font_large;
static GFont s_font_medium;
static GFont s_font_small;
static GFont s_font_title;

// ============================================================
// Canvas: squircle bezel, dividers, BT icon, bolt icon, date box
// ============================================================
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int cx = bounds.size.w / 2;

  // --- Squircle bezel tick marks ---
  for (int i = 0; i < BEZEL_NUM_TICKS; i++) {
    GPoint p_outer, p_inner;
    if (i % 5 == 0) {
      graphics_context_set_stroke_color(ctx, GColorCyan);
      p_outer = GPoint(s_bezel_long[i][0], s_bezel_long[i][1]);
      p_inner = GPoint(s_bezel_long[i][2], s_bezel_long[i][3]);
    } else {
      graphics_context_set_stroke_color(ctx, GColorWhite);
      p_outer = GPoint(s_bezel_short[i][0], s_bezel_short[i][1]);
      p_inner = GPoint(s_bezel_short[i][2], s_bezel_short[i][3]);
    }
    graphics_draw_line(ctx, p_outer, p_inner);
  }

  // --- Vertical divider between left/right columns (upper section) ---
  graphics_context_set_stroke_color(ctx, GColorCyan);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(cx, 38), GPoint(cx, 82));

  // --- Date number box (filled cyan, text drawn black on top) ---
  graphics_context_set_fill_color(ctx, GColorCyan);
  graphics_fill_rect(ctx, GRect(59, 56, 32, 28), 0, GCornerNone);

  // --- Vertical divider between STEPS and KCAL ---
  graphics_draw_line(ctx, GPoint(cx, 149), GPoint(cx, 189));
  graphics_context_set_stroke_width(ctx, 1);

  // --- Bluetooth icon: two interlocking chain links ---
  if (s_bt_connected) {
    graphics_context_set_stroke_color(ctx, GColorCyan);
    graphics_context_set_stroke_width(ctx, 2);
    // Left link (rounded rect outline)
    graphics_draw_round_rect(ctx, GRect(48, 202, 12, 9), 2);
    // Right link (rounded rect outline, overlapping)
    graphics_draw_round_rect(ctx, GRect(56, 205, 12, 9), 2);
    graphics_context_set_stroke_width(ctx, 1);
  }

  // --- Lightning bolt icon ---
  graphics_context_set_fill_color(ctx, GColorCyan);
  GPoint bolt_points[] = {
    {103, 202}, {99, 209}, {102, 209},
    {98, 216}, {105, 207}, {102, 207}, {103, 202}
  };
  GPathInfo bolt_info = { .num_points = 7, .points = bolt_points };
  GPath *bolt_path = gpath_create(&bolt_info);
  gpath_draw_filled(ctx, bolt_path);
  gpath_destroy(bolt_path);
}

static TextLayer *create_text_layer(Layer *parent, GRect frame, GFont font,
                                     GColor text_color, GTextAlignment align) {
  TextLayer *tl = text_layer_create(frame);
  text_layer_set_background_color(tl, GColorClear);
  text_layer_set_text_color(tl, text_color);
  text_layer_set_font(tl, font);
  text_layer_set_text_alignment(tl, align);
  layer_add_child(parent, text_layer_get_layer(tl));
  return tl;
}

static void update_time(struct tm *tick_time) {
  if (clock_is_24h_style()) {
    strftime(s_hours_buf, sizeof(s_hours_buf), "%H", tick_time);
    s_ampm_buf[0] = '\0';
  } else {
    strftime(s_hours_buf, sizeof(s_hours_buf), "%I", tick_time);
    strftime(s_ampm_buf, sizeof(s_ampm_buf), "%p", tick_time);
  }
  strftime(s_minutes_buf, sizeof(s_minutes_buf), "%M", tick_time);
  text_layer_set_text(s_hours_layer, s_hours_buf);
  text_layer_set_text(s_minutes_layer, s_minutes_buf);
  text_layer_set_text(s_ampm_layer, s_ampm_buf);
}

static void update_date(struct tm *tick_time) {
  strftime(s_day_buf, sizeof(s_day_buf), "%a", tick_time);
  strftime(s_month_buf, sizeof(s_month_buf), "%b", tick_time);
  strftime(s_date_num_buf, sizeof(s_date_num_buf), "%d", tick_time);
  for (int i = 0; s_day_buf[i]; i++)
    if (s_day_buf[i] >= 'a' && s_day_buf[i] <= 'z') s_day_buf[i] -= 32;
  for (int i = 0; s_month_buf[i]; i++)
    if (s_month_buf[i] >= 'a' && s_month_buf[i] <= 'z') s_month_buf[i] -= 32;
  text_layer_set_text(s_day_layer, s_day_buf);
  text_layer_set_text(s_month_layer, s_month_buf);
  text_layer_set_text(s_date_num_layer, s_date_num_buf);
}

static void update_weather_icon() {
  GBitmap *bmp = NULL;
  switch (s_weather_condition) {
    case WEATHER_SUNNY:  bmp = s_weather_sunny_bmp; break;
    case WEATHER_CLOUDY: bmp = s_weather_cloudy_bmp; break;
    case WEATHER_RAINY:  bmp = s_weather_rainy_bmp; break;
    case WEATHER_SNOWY:  bmp = s_weather_snowy_bmp; break;
    case WEATHER_THUNDER: bmp = s_weather_thunder_bmp; break;
    default: bmp = s_weather_sunny_bmp; break;
  }
  bitmap_layer_set_bitmap(s_weather_icon_layer, bmp);
}

static int owm_condition_to_icon(int code) {
  if (code >= 200 && code < 300) return WEATHER_THUNDER;
  if (code >= 300 && code < 400) return WEATHER_RAINY;
  if (code >= 500 && code < 600) return WEATHER_RAINY;
  if (code >= 600 && code < 700) return WEATHER_SNOWY;
  if (code >= 700 && code < 800) return WEATHER_CLOUDY;
  if (code == 800) return WEATHER_SUNNY;
  if (code > 800) return WEATHER_CLOUDY;
  return WEATHER_SUNNY;
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
  if (units_changed & DAY_UNIT) update_date(tick_time);
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, 0, 0);
    app_message_outbox_send();
  }
}

static void battery_handler(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  s_battery_charging = state.is_charging;
  snprintf(s_battery_buf, sizeof(s_battery_buf), "%d %%", s_battery_level);
  text_layer_set_text(s_battery_layer, s_battery_buf);
}

static void bt_handler(bool connected) {
  if (!connected && s_bt_connected) vibes_double_pulse();
  s_bt_connected = connected;
  layer_mark_dirty(s_canvas_layer);
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    snprintf(s_steps_buf, sizeof(s_steps_buf), "%d", (int)steps);
    text_layer_set_text(s_steps_value_layer, s_steps_buf);
    HealthValue kcal = health_service_sum_today(HealthMetricActiveKCalories);
    snprintf(s_kcal_buf, sizeof(s_kcal_buf), "%d", (int)kcal);
    text_layer_set_text(s_kcal_value_layer, s_kcal_buf);
  }
  if (event == HealthEventHeartRateUpdate) {
    HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if (hr > 0) {
      snprintf(s_bpm_buf, sizeof(s_bpm_buf), "%d", (int)hr);
    } else {
      snprintf(s_bpm_buf, sizeof(s_bpm_buf), "--");
    }
    text_layer_set_text(s_bpm_value_layer, s_bpm_buf);
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_KEY_TEMPERATURE);
  Tuple *cond_tuple = dict_find(iterator, MESSAGE_KEY_KEY_CONDITIONS);
  if (temp_tuple) {
    snprintf(s_temp_buf, sizeof(s_temp_buf), "%d°C", (int)temp_tuple->value->int32);
    text_layer_set_text(s_temp_layer, s_temp_buf);
  }
  if (cond_tuple) {
    s_weather_condition = owm_condition_to_icon((int)cond_tuple->value->int32);
    update_weather_icon();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                    AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed: %d", (int)reason);
}

// ============================================================
// Window load
// ============================================================
static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  window_set_background_color(window, GColorBlack);

  // --- Custom fonts (Montserrat + Oswald from Google Fonts) ---
  s_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANKEN_BOLD_58));
  s_font_large = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANKEN_MEDIUM_22));
  s_font_medium = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANKEN_MEDIUM_16));
  s_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANKEN_MEDIUM_14));
  s_font_title = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FIRA_EXTRACOND_16));

  // --- Bitmaps ---
  s_weather_sunny_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEATHER_SUNNY);
  s_weather_cloudy_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEATHER_CLOUDY);
  s_weather_rainy_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEATHER_RAINY);
  s_weather_snowy_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEATHER_SNOWY);
  s_weather_thunder_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEATHER_THUNDER);
  s_heart_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART);
  s_steps_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STEPS);
  s_calories_bmp = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CALORIES);

  // --- Canvas layer (bezel, dividers, procedural icons) ---
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);

  // --- Title: "SMART WATCH" (Oswald Bold 16, cyan) ---
  s_title_layer = create_text_layer(root, GRect(0, 12, SCREEN_W, 22),
                                     s_font_title, GColorCyan, GTextAlignmentCenter);
  text_layer_set_text(s_title_layer, "SMART WATCH");

  // --- Weather icon (above date box, centered over it) ---
  s_weather_icon_layer = bitmap_layer_create(GRect(62, 32, 25, 25));
  bitmap_layer_set_compositing_mode(s_weather_icon_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_sunny_bmp);
  layer_add_child(root, bitmap_layer_get_layer(s_weather_icon_layer));

  // --- Temperature (right column, starts at heart center x=123) ---
  s_temp_layer = create_text_layer(root, GRect(120, 34, 70, 28),
                                    s_font_large, GColorWhite, GTextAlignmentLeft);
  snprintf(s_temp_buf, sizeof(s_temp_buf), "--°C");
  text_layer_set_text(s_temp_layer, s_temp_buf);

  // --- Day of week (left column, vertically centered with date box) ---
  s_day_layer = create_text_layer(root, GRect(13, 52, 44, 20),
                                   s_font_medium, GColorLightGray, GTextAlignmentRight);

  // --- Month (left column, just below day, aligned with box) ---
  s_month_layer = create_text_layer(root, GRect(13, 67, 44, 20),
                                     s_font_medium, GColorLightGray, GTextAlignmentRight);

  // --- Date number (black text on cyan filled box) ---
  s_date_num_layer = create_text_layer(root, GRect(59, 56, 32, 28),
                                        s_font_large, GColorBlack, GTextAlignmentCenter);

  // --- Heart icon (right column, 30x30) ---
  s_heart_icon_layer = bitmap_layer_create(GRect(108, 56, 30, 30));
  bitmap_layer_set_compositing_mode(s_heart_icon_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_heart_icon_layer, s_heart_bmp);
  layer_add_child(root, bitmap_layer_get_layer(s_heart_icon_layer));

  // --- BPM label (GREY, Gothic 14 Bold) ---
  s_bpm_label_layer = create_text_layer(root, GRect(140, 59, 45, 16),
                                         s_font_small, GColorLightGray, GTextAlignmentLeft);
  text_layer_set_text(s_bpm_label_layer, "BPM");

  // --- BPM value (WHITE, Gothic 18 Bold) ---
  s_bpm_value_layer = create_text_layer(root, GRect(108, 72, 80, 20),
                                         s_font_medium, GColorWhite, GTextAlignmentCenter);
  snprintf(s_bpm_buf, sizeof(s_bpm_buf), "--");
  text_layer_set_text(s_bpm_value_layer, s_bpm_buf);

  // --- Hours (large, WHITE, Bitham 42 - centered on 3-9 axis) ---
  s_hours_layer = create_text_layer(root, GRect(0, 78, 86, 64),
                                     s_font_time, GColorWhite, GTextAlignmentRight);

  // --- AM/PM (cyan, 12pt) ---
  GFont font_ampm = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANKEN_MEDIUM_12));
  s_ampm_layer = create_text_layer(root, GRect(93, 92, 25, 14),
                                    font_ampm, GColorWhite, GTextAlignmentLeft);

  // --- Colon (cyan) ---
  s_colon_layer = create_text_layer(root, GRect(88, 78, 24, 64),
                                     s_font_time, GColorCyan, GTextAlignmentCenter);
  text_layer_set_text(s_colon_layer, ":");

  // --- Minutes (large, CYAN) ---
  s_minutes_layer = create_text_layer(root, GRect(112, 78, 88, 64),
                                       s_font_time, GColorCyan, GTextAlignmentLeft);

  // --- Steps icon (moved up) ---
  s_steps_icon_layer = bitmap_layer_create(GRect(22, 146, 25, 25));
  bitmap_layer_set_compositing_mode(s_steps_icon_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_steps_icon_layer, s_steps_bmp);
  layer_add_child(root, bitmap_layer_get_layer(s_steps_icon_layer));

  // --- Steps label (GREY, Gothic 14 Bold) ---
  s_steps_label_layer = create_text_layer(root, GRect(48, 149, 50, 16),
                                           s_font_small, GColorLightGray, GTextAlignmentLeft);
  text_layer_set_text(s_steps_label_layer, "STEPS");

  // --- Steps value (WHITE, Gothic 24 Bold) ---
  s_steps_value_layer = create_text_layer(root, GRect(15, 167, 82, 28),
                                           s_font_large, GColorWhite, GTextAlignmentCenter);
  snprintf(s_steps_buf, sizeof(s_steps_buf), "0");
  text_layer_set_text(s_steps_value_layer, s_steps_buf);

  // --- Calories icon (moved up) ---
  s_calories_icon_layer = bitmap_layer_create(GRect(110, 146, 25, 25));
  bitmap_layer_set_compositing_mode(s_calories_icon_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_calories_icon_layer, s_calories_bmp);
  layer_add_child(root, bitmap_layer_get_layer(s_calories_icon_layer));

  // --- KCAL label (GREY, Gothic 14 Bold) ---
  s_kcal_label_layer = create_text_layer(root, GRect(136, 149, 50, 16),
                                          s_font_small, GColorLightGray, GTextAlignmentLeft);
  text_layer_set_text(s_kcal_label_layer, "KCAL");

  // --- KCAL value (WHITE, Gothic 24 Bold) ---
  s_kcal_value_layer = create_text_layer(root, GRect(103, 167, 82, 28),
                                          s_font_large, GColorWhite, GTextAlignmentCenter);
  snprintf(s_kcal_buf, sizeof(s_kcal_buf), "0");
  text_layer_set_text(s_kcal_value_layer, s_kcal_buf);

  // --- Battery percentage (WHITE, under KCAL, even with BT icon) ---
  s_battery_layer = create_text_layer(root, GRect(125, 196, 70, 22),
                                       s_font_medium, GColorWhite, GTextAlignmentLeft);
  snprintf(s_battery_buf, sizeof(s_battery_buf), "0 %%");
  text_layer_set_text(s_battery_layer, s_battery_buf);

  // --- Initialize data ---
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time(t);
  update_date(t);
  battery_handler(battery_state_service_peek());
  s_bt_connected = connection_service_peek_pebble_app_connection();

  if (health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), now)) {
    HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    snprintf(s_steps_buf, sizeof(s_steps_buf), "%d", (int)steps);
    text_layer_set_text(s_steps_value_layer, s_steps_buf);
  }
  if (health_service_metric_accessible(HealthMetricActiveKCalories, time_start_of_today(), now)) {
    HealthValue kcal = health_service_sum_today(HealthMetricActiveKCalories);
    snprintf(s_kcal_buf, sizeof(s_kcal_buf), "%d", (int)kcal);
    text_layer_set_text(s_kcal_value_layer, s_kcal_buf);
  }
}

static void window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_temp_layer);
  text_layer_destroy(s_day_layer);
  text_layer_destroy(s_month_layer);
  text_layer_destroy(s_date_num_layer);
  text_layer_destroy(s_bpm_label_layer);
  text_layer_destroy(s_bpm_value_layer);
  text_layer_destroy(s_hours_layer);
  text_layer_destroy(s_minutes_layer);
  text_layer_destroy(s_ampm_layer);
  text_layer_destroy(s_colon_layer);
  text_layer_destroy(s_steps_label_layer);
  text_layer_destroy(s_steps_value_layer);
  text_layer_destroy(s_kcal_label_layer);
  text_layer_destroy(s_kcal_value_layer);
  text_layer_destroy(s_battery_layer);
  bitmap_layer_destroy(s_weather_icon_layer);
  bitmap_layer_destroy(s_heart_icon_layer);
  bitmap_layer_destroy(s_steps_icon_layer);
  bitmap_layer_destroy(s_calories_icon_layer);
  gbitmap_destroy(s_weather_sunny_bmp);
  gbitmap_destroy(s_weather_cloudy_bmp);
  gbitmap_destroy(s_weather_rainy_bmp);
  gbitmap_destroy(s_weather_snowy_bmp);
  gbitmap_destroy(s_weather_thunder_bmp);
  gbitmap_destroy(s_heart_bmp);
  gbitmap_destroy(s_steps_bmp);
  gbitmap_destroy(s_calories_bmp);
  layer_destroy(s_canvas_layer);
  fonts_unload_custom_font(s_font_time);
  fonts_unload_custom_font(s_font_large);
  fonts_unload_custom_font(s_font_medium);
  fonts_unload_custom_font(s_font_small);
  fonts_unload_custom_font(s_font_title);
}

static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load, .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_handler,
  });
  health_service_events_subscribe(health_handler, NULL);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_open(128, 128);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  health_service_events_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
