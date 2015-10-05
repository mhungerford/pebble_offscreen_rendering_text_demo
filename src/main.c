#include <pebble.h>

#include "fireworks.h"

static Window *my_window;

// if user is looking at watch, start glance motion
static bool looking = false;
static AppTimer *glancing_timer_handle = NULL;
static BitmapLayer *render_layer = NULL;
static GBitmap *render_bitmap = NULL;


GFont lcd_date_font = NULL;
GFont lcd_time_font = NULL;
GBitmap *mask = NULL;

//Digital Time Display
char time_string[] = "00:00";  // Make this longer to show AM/PM
Layer *digital_layer = NULL;

//Digital Date Display
char date_wday_string[] = "WED";
char date_mday_string[] = "30";
Layer *date_layer = NULL;
static const char *const dname[7] =
{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

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

static void digital_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  GPoint box_point = grect_center_point(&bounds);

  // Size box to width of wday
  GSize box_size = graphics_text_layout_get_content_size(
      time_string, lcd_time_font, bounds,
      GTextOverflowModeWordWrap, GTextAlignmentCenter);
  box_size.w += 2; // Padding

  graphics_draw_bitmap_in_rect(ctx, mask, 
      GRect(box_point.x - box_size.w / 2, bounds.origin.y, box_size.w, bounds.size.h));

  graphics_context_set_text_color(ctx, GColorCyan);
  graphics_draw_text(ctx, time_string, lcd_time_font, 
      GRect( 
        bounds.origin.x + 2, bounds.origin.y - 2,
        bounds.size.w, bounds.size.h - 2),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  GPoint box_point = grect_center_point(&bounds);

  // Size box to width of wday
  GSize box_size = graphics_text_layout_get_content_size(
      date_wday_string, lcd_date_font, bounds,
      GTextOverflowModeWordWrap, GTextAlignmentCenter);
  box_size.w += 4; // Padding

  graphics_draw_bitmap_in_rect(ctx, mask, 
      GRect(box_point.x - box_size.w / 2, bounds.origin.y, box_size.w, bounds.size.h));

  graphics_context_set_text_color(ctx, GColorCyan);
  graphics_draw_text(ctx, date_wday_string, lcd_date_font,
      GRect( 
        bounds.origin.x + 2, bounds.origin.y - 2,
        bounds.size.w, bounds.size.h),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  graphics_draw_text(ctx, date_mday_string, lcd_date_font,
      GRect( 
        bounds.origin.x + 2, bounds.origin.y + 20 - 2,
        bounds.size.w, bounds.size.h),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

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
  if (units_changed & DAY_UNIT) {
    time_t current_time = time(NULL);
    struct tm *current_tm = localtime(&current_time);
    snprintf(date_wday_string, sizeof(date_wday_string), "%s", dname[current_tm->tm_wday]);
    snprintf(date_mday_string, sizeof(date_mday_string), "%d", current_tm->tm_mday);
    layer_mark_dirty(date_layer);
  }
  clock_copy_time_string(time_string,sizeof(time_string));
  // Remove the space on the end of the string in AM/PM mode
  if (strchr(time_string, ' ')) {
    time_string[strlen(time_string) - 1] = '\0';
  }
  layer_mark_dirty(digital_layer);
}

static void glance_timer(void* data) {
  looking = false;
}

static void register_timer(void* data) {
  if (looking) {
    app_timer_register(50, register_timer, data);
    Firework_Update(render_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(render_layer));
  }
}

static void light_timer(void *data) {
  if (looking) {
    app_timer_register(2 * 1000, light_timer, data);
    light_enable_interaction();
  }
}

#define ACCEL_DEADZONE 100
#define WITHIN(n, min, max) (((n)>=(min) && (n) <= (max)) ? true : false)

static int unglanced = true;

void accel_handler(AccelData *data, uint32_t num_samples) {
  for (uint32_t i = 0; i < num_samples; i++) {
    if(
        WITHIN(data[i].x, -400, 400) &&
        WITHIN(data[i].y, -900, 0) &&
        WITHIN(data[i].z, -1100, -300)) {
      if (unglanced) {
        unglanced = false;
        looking = true;
        register_timer(NULL);
        // turn glancing off in 40 seconds
        glancing_timer_handle = app_timer_register(40 * 1000, glance_timer, data);
        light_timer(NULL);
      }
      return;
    }
  }
  if (glancing_timer_handle) {
    app_timer_cancel(glancing_timer_handle);
  }
  unglanced = true;
  looking = false;
}

void tap_handler(AccelAxisType axis, int32_t direction){
  if(!looking) {
    // force light to be off when we are not looking
    // ie. save power if accidental flick angles
    light_enable(false);
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const GPoint center = grect_center_point(&bounds);
  

  // Setup render layer for fireworks
  Firework_Initialize(bounds.size.w, bounds.size.h);

  render_layer = bitmap_layer_create(bounds);
  render_bitmap = gbitmap_create_blank(bounds.size, GBitmapFormat8Bit);
  bitmap_layer_set_bitmap(render_layer, render_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(render_layer));
  //register_timer(NULL);

  custom_font_text = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_TEXT_20));
  custom_font_outline = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_OUTLINE_20));
  mask = gbitmap_create_with_resource(RESOURCE_ID_MASK);
  
  //Add layers from back to front (background first)

  //Load the lcd font
  lcd_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LCD_24));
  lcd_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LCD_20));

  //Setup background layer for digital time display
  digital_layer = layer_create(GRect(center.x - 32, center.y + 22, 32 * 2, 24));
  layer_set_update_proc(digital_layer, digital_update_proc);
  layer_add_child(window_layer, digital_layer);

  //Setup background layer for digital date display
  date_layer = layer_create(GRect(center.x - 20, center.y - 56, 20 * 2, 40));
  layer_set_update_proc(date_layer, date_update_proc);
  layer_add_child(window_layer, date_layer);

  //Add analog hands
  analog_init(my_window);

  //Force time update
  time_t current_time = time(NULL);
  struct tm *current_tm = localtime(&current_time);
  tick_handler(current_tm, MINUTE_UNIT | DAY_UNIT);

  //Setup tick time handler
  tick_timer_service_subscribe((MINUTE_UNIT), tick_handler);
  
  //Setup magic motion accel handler
  accel_data_service_subscribe(5, accel_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);

  //light_enable(true);
  //looking = true;
  //register_timer(NULL);

  //Setup tap service to avoid old flick to light behavior
  accel_tap_service_subscribe(tap_handler);
}

static void window_unload(Window *window) {
  //bitmap_layer_destroy(render_layer);
  //gbitmap_destroy(render_bitmap);
	//window_destroy(my_window);
}

static void init(void) {
  my_window = window_create();
  //window_set_fullscreen(window, true);
  window_set_background_color(my_window, GColorBlack);
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
