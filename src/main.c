#include <pebble.h>

#include "offscreen.h"

static Window *my_window;

static Layer *render_layer = NULL;

static Layer *background_layer = NULL;

static GPath *minute_arrow;
static GPath *minute_fill;

static GPath *hour_arrow;
static GPath *hour_fill;
static GPath *peg_fill;

static GPath *tick_path_1;
static GPath *tick_path_3;
static GPath *tick_path_5;
static GPath *tick_path_7;
static GPath *tick_path_9;
static GPath *tick_path_11;

static GPoint tick_point_2;
static GPoint tick_point_4;
static GPoint tick_point_6;
static GPoint tick_point_8;
static GPoint tick_point_10;
static GPoint tick_point_12;

#define CAT(a, b) a##b
#define STR(s) #s
#define draw_hour(hour) \
  graphics_context_set_text_color(ctx, GColorBlack); \
  graphics_draw_text(ctx, STR(hour), custom_font_outline, (GRect){.origin = \
      (GPoint){CAT(tick_point_, hour).x - 17, CAT(tick_point_, hour).y - 11}, .size = {32,32}}, \
      GTextOverflowModeFill, GTextAlignmentCenter, NULL); \
  graphics_context_set_text_color(ctx, GColorWhite); \
  graphics_draw_text(ctx, STR(hour), custom_font_text, (GRect){.origin = \
      (GPoint){CAT(tick_point_, hour).x - 16, CAT(tick_point_, hour).y - 10}, .size = {32,32}}, \
      GTextOverflowModeFill, GTextAlignmentCenter, NULL)

static GFont custom_font_text = NULL;
static GFont custom_font_outline = NULL;

static Layer *analog_layer;

static const GPathInfo MINUTE_HAND_POINTS = {6, (GPoint []) {
    { -6,  15 },
    {  0,  20 },
    {  6,  15 },
    {  4, -56 },
    {  0, -66 },
    { -4, -56 }
  }
};

static const GPathInfo MINUTE_FILL_POINTS = {5, (GPoint []) {
    { -6,  15 },
    {  0,  20 },
    {  6,  15 },
    {  4, -8 },
    { -4, -8 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {6, (GPoint []) {
    { -7,  12},
    { -0,  15},
    {  7,  12},
    {  5, -42},
    {  0, -52},
    { -5, -42}
  }
};

static const GPathInfo HOUR_FILL_POINTS = {5, (GPoint []) {
    { -7,  12},
    {  0,  15},
    {  7,  12},
    {  7, -8},
    { -7, -8},
  }
};

static const GPathInfo PEG_POINTS = {6, (GPoint []) {
  // Start at the top point, clockwise
    {  0,  5},
    {  5,  2},
    {  5, -2},
    {  0, -5},
    { -5, -2},
    { -5,  2},
  }
};

#define TICK_DISTANCE 72
static const GPathInfo TICK_POINTS = {4, (GPoint []) {
    {  3,  7},
    { -3,  7},
    { -5, -9},
    {  5, -9},
  }
};

// local prototypes
static void draw_ticks(Layer *layer, GContext *ctx);

static void analog_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // Draw ticks behind
  draw_ticks(layer, ctx);

  // minute/hour hand
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_context_set_antialiased(ctx, true);

  gpath_rotate_to(minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_rotate_to(minute_fill, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_rotate_to(hour_arrow, 
      (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10)))/(12 * 6));
  gpath_rotate_to(hour_fill, 
      (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10)))/(12 * 6));

  // Draw minute hand background
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 6);
  gpath_draw_outline(ctx, minute_arrow);

  // Draw minute hand foreground
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 3);
  gpath_draw_outline(ctx, minute_arrow);
  gpath_draw_filled(ctx, minute_fill);

  // Draw hour hand background
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 6);
  gpath_draw_outline(ctx, hour_arrow);

  // Draw hour hand foreground
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 3);
  gpath_draw_outline(ctx, hour_arrow);
  gpath_draw_filled(ctx, hour_fill);

  // Draw a black peg in the center
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  gpath_draw_filled(ctx, peg_fill);
}

static void draw_ticks(Layer *layer, GContext *ctx) {
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 5);
  
  gpath_draw_outline(ctx, tick_path_1);
  gpath_draw_filled(ctx, tick_path_1);

  gpath_draw_outline(ctx, tick_path_3);
  gpath_draw_filled(ctx, tick_path_3);
  
  gpath_draw_outline(ctx, tick_path_5);
  gpath_draw_filled(ctx, tick_path_5);

  gpath_draw_outline(ctx, tick_path_7);
  gpath_draw_filled(ctx, tick_path_7);
  
  gpath_draw_outline(ctx, tick_path_9);
  gpath_draw_filled(ctx, tick_path_9);
  
  gpath_draw_outline(ctx, tick_path_11);
  gpath_draw_filled(ctx, tick_path_11);

  draw_hour(2);
  draw_hour(4);
  draw_hour(6);
  draw_hour(8);
  draw_hour(10);
  draw_hour(12);
}

GPoint tick_point(GPoint center, uint16_t hour) {
  return (GPoint){
      center.x + TICK_DISTANCE * sin_lookup(TRIG_MAX_ANGLE * hour / 12) / TRIG_MAX_RATIO, 
      center.y - TICK_DISTANCE * cos_lookup(TRIG_MAX_ANGLE * hour / 12) / TRIG_MAX_RATIO};
}

void tick_setup(GPath **path, GPoint center, uint16_t hour) {
  *path = gpath_create(&TICK_POINTS);
  gpath_rotate_to(*path, TRIG_MAX_ANGLE * hour / 12);
  gpath_move_to(*path, tick_point(center, hour));
}


void tick_init(Window* window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const GPoint center = grect_center_point(&bounds);

  tick_setup(&tick_path_1, center, 1);
  tick_setup(&tick_path_3, center, 3);
  tick_setup(&tick_path_5, center, 5);
  tick_setup(&tick_path_7, center, 7);
  tick_setup(&tick_path_9, center, 9);
  tick_setup(&tick_path_11, center, 11);

  tick_point_2 = tick_point(center, 2);
  tick_point_4 = tick_point(center, 4);
  tick_point_6 = tick_point(center, 6);
  tick_point_8 = tick_point(center, 8);
  tick_point_10 = tick_point(center, 10);
  tick_point_12 = tick_point(center, 12);
}

void analog_init(Window* window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const GPoint center = grect_center_point(&bounds);

  analog_layer = layer_create(bounds);
  layer_set_update_proc(analog_layer, analog_update_proc);
  layer_add_child(window_layer, analog_layer);

  minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  minute_fill = gpath_create(&MINUTE_FILL_POINTS);
  hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  hour_fill = gpath_create(&HOUR_FILL_POINTS);
  peg_fill = gpath_create(&PEG_POINTS);

  gpath_move_to(minute_arrow, center);
  gpath_move_to(minute_fill, center);
  gpath_move_to(hour_arrow, center);
  gpath_move_to(hour_fill, center);
  gpath_move_to(peg_fill, center);

  tick_init(window);
}

void analog_destroy(void) {
  layer_destroy(analog_layer);
  gpath_destroy(minute_arrow);
  gpath_destroy(hour_arrow);
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  Layer *window_layer = window_get_root_layer(my_window);
  layer_mark_dirty(window_layer);
}

static void digital_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
    
  time_t current_time = time(NULL);
  struct tm *time_tm = localtime(&current_time);
  char time_string[] = "00:00:00";
  strftime(time_string, sizeof(time_string), 
    clock_is_24h_style() ? "%H:%M" : "%I:%M", time_tm);
  
  char date_string[] = "DAYNAME";
  strftime(date_string, sizeof(date_string), "%a", time_tm);

  graphics_context_set_text_color(ctx, GColorCyan);
  GFont date_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  graphics_draw_text(ctx, date_string, date_font, GRect(0,30, 180, 32),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  graphics_context_set_text_color(ctx, GColorYellow);
  GFont time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  graphics_draw_text(ctx, time_string, time_font, GRect(0,90, 180, 44),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void background_layer_update(Layer* layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GBitmap *bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_1);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, bitmap, bounds);
  gbitmap_destroy(bitmap);
}

static void render_layer_update(Layer* layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  //backup old context data pointer
  uint8_t *orig_addr = gbitmap_get_data((GBitmap*)ctx);
  GBitmapFormat orig_format = gbitmap_get_format((GBitmap*)ctx);
  uint16_t orig_stride = gbitmap_get_bytes_per_row((GBitmap*)ctx);

#if 0 // DP7+
  GBitmap *render_bitmap = gbitmap_create_blank(bounds.size, GBitmapFormat8BitCircular);
#else
  GBitmap *render_bitmap = gbitmap_create_blank(bounds.size, GBitmapFormat8Bit);
#endif

  //replace screen bitmap with our offscreen render bitmap
  gbitmap_set_data((GBitmap*)ctx, gbitmap_get_data(render_bitmap),
    gbitmap_get_format(render_bitmap), gbitmap_get_bytes_per_row(render_bitmap), false);

  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  digital_update_proc(layer, ctx);
  offscreen_make_transparent(ctx);

  //restore original context bitmap
  gbitmap_set_data((GBitmap*)ctx, orig_addr, orig_format, orig_stride, false);

  //draw the bitmap to the screen
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, render_bitmap, bounds);
  gbitmap_destroy(render_bitmap);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const GPoint center = grect_center_point(&bounds);
  
  //Load bitmap image
  background_layer = layer_create(bounds);
  layer_set_update_proc(background_layer, background_layer_update);
  layer_add_child(window_layer, background_layer);

  custom_font_text = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_TEXT_20));
  custom_font_outline = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_OUTLINE_20));
  
  //Add analog hands
  analog_init(my_window);

  // Render layer ontop for cool transparency
  render_layer = layer_create(bounds);
  layer_set_update_proc(render_layer, render_layer_update);
  layer_add_child(window_layer, render_layer);

  //Force time update
  time_t current_time = time(NULL);
  struct tm *current_tm = localtime(&current_time);
  tick_handler(current_tm, MINUTE_UNIT | DAY_UNIT);

  //Setup tick time handler
  tick_timer_service_subscribe((MINUTE_UNIT), tick_handler);
}

static void window_unload(Window *window) {
  //bitmap_layer_destroy(render_layer);
  //gbitmap_destroy(render_bitmap);
	//window_destroy(my_window);
}

static void init(void) {
  my_window = window_create();
  window_set_background_color(my_window, GColorBlue);
  window_set_window_handlers(my_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
      });
  window_stack_push(my_window, false);
}

static void deinit(void) {
  //window_destroy(my_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
