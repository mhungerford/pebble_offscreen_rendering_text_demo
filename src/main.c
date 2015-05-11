#include <pebble.h>

#define USE_FIXED_POINT 1

#ifdef USE_FIXED_POINT
  #include "math-sll.h"
#else
  #define CONST_PI M_PI
#endif



//Time Display
char time_string[] = "00:00";  // Make this longer to show AM/PM
TextLayer* time_outline_layer = NULL;
TextLayer* time_text_layer = NULL;

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

static GFont *custom_font_text = NULL;
static GFont *custom_font_outline = NULL;

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

  layer_set_hidden(text_layer_get_layer(time_outline_layer), true);
  layer_set_hidden(text_layer_get_layer(time_text_layer), true);

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
  clock_copy_time_string(time_string,sizeof(time_string));
  layer_mark_dirty(text_layer_get_layer(time_outline_layer));
  layer_mark_dirty(text_layer_get_layer(time_text_layer));
}


#define COLOR(x) {.argb=GColor ## x ## ARGB8}

typedef struct Pattern {
  int R;
  int r;
  int d;
} Pattern;

Pattern patterns[]= {
  {65, 15, 24},  // big lobes
  {95, 55, 12},  // ball
  {65, -15, 24}, // offscreen
  {95, 55, 42},  // offspiral
  {15, 55, 45},  // tight ball
  {25, 55, 25},  // lopsided ball
  {35, 55, 35},  // very cool 
};

GColor background_colors[] = {
  COLOR(Black), 
  COLOR(Blue), 
  COLOR(DukeBlue),
  COLOR(OxfordBlue),
  COLOR(DarkGreen),
  COLOR(ImperialPurple),
};

bool draw_spirograph(GContext* ctx, int x, int y, int outer, int inner, int dist) {
  //int t = 144 / 2; // (width / 2)

  static GPoint point = {0,0};
  static GPoint first_point = {0,0};
  static GColor color;
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_antialiased(ctx, true);

  sll R = int2sll(outer);
  sll r = int2sll(inner);
  sll d = int2sll(dist);
  sll q = sllsub(R, r);
  sll xip = slldiv(CONST_PI, sllmul(CONST_2, sllexp(CONST_4))); // PI / 2e^4
  sll R_pi = sllmul(R, CONST_PI);

  static sll index = CONST_0;

  int segments = 0;

  while(index < R_pi) {
    index = slladd(index, xip);
    sll iq_over_r = slldiv(sllmul(index, q), r); // i * (q / r)
    int xval = 
      sll2int(sllmul(q, sllsin(index))) - sll2int(sllmul(d, sllsin(iq_over_r)));
    int yval = 
      sll2int(sllmul(q, sllcos(index))) + sll2int(sllmul(d, sllcos(iq_over_r)));

    // If we are back to the starting point, quit
    if (gpoint_equal(&(GPoint){xval, yval}, &first_point)) {
      index = CONST_0;
      point = GPointZero;
      return false;
    }

    if (!gpoint_equal(&point, &GPointZero)) {
      graphics_draw_line(ctx, (GPoint){point.x + x, point.y + y}, (GPoint){xval + x, yval + y});
    } else {
      first_point = (GPoint){xval, yval};
    }

    point = (GPoint){xval, yval};

    if (segments++ >= 10) {
      color.argb = (color.argb + 1 ) | 0xc0;
      break;
    }
  }

  if (index > R_pi) {
    index = CONST_0;
    point = GPointZero;
    return false;
  }

  return true;
}

static void update_display(Layer* layer, GContext *ctx) {
  bool retval = true;
  // Always set composite mode for window before drawing
  // acts as an accumulation buffer for drawing over time
  window_set_background_color(window_stack_get_top_window(), GColorClear);
  static int idx = 0;
  int num_patterns = sizeof(patterns) / sizeof(Pattern);

  retval = draw_spirograph(ctx, 72, 84, 
      patterns[idx].R, patterns[idx].r, patterns[idx].d);

  if (!retval) {
    //reset the background color
    window_set_background_color(window_stack_get_top_window(), 
        background_colors[rand() % (sizeof(background_colors) / sizeof(GColor))]);
    //change the pattern randomly
    //while (last_idx == idx) {
    //  idx = rand() % num_patterns;
    //}
    idx = (idx + 1) % num_patterns;
  }
}

static Window *window;
static Layer *render_layer;

static void register_timer(void* data) {
  app_timer_register(50, register_timer, data);
  layer_mark_dirty(render_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  render_layer = layer_create(bounds);
  layer_set_update_proc(render_layer, update_display);
  layer_add_child(window_layer, render_layer);
  register_timer(NULL);

  custom_font_text = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_TEXT_20));
  custom_font_outline = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_OUTLINE_20));
  
  //Add layers from back to front (background first)

  //Setup the time outline display
  time_outline_layer = text_layer_create(GRect(0, 12, 144, 30));
  text_layer_set_text(time_outline_layer, time_string);
	text_layer_set_font(time_outline_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_OUTLINE_20)));
  text_layer_set_text_alignment(time_outline_layer, GTextAlignmentCenter);
  text_layer_set_background_color(time_outline_layer, GColorClear);
  text_layer_set_text_color(time_outline_layer, GColorBlack);
  
  //Add clock text second
  layer_add_child(window_layer, text_layer_get_layer(time_outline_layer));

  //Setup the time display
  time_text_layer = text_layer_create(GRect(0, 12, 144, 30));
  text_layer_set_text(time_text_layer, time_string);
	text_layer_set_font(time_text_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOXY_TEXT_20)));
  text_layer_set_text_alignment(time_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(time_text_layer, GColorClear);
  text_layer_set_text_color(time_text_layer, GColorWhite);
  
  //Add clock text second
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));

  //Add analog hands
  analog_init(window);

  //Force time update
  time_t current_time = time(NULL);
  struct tm *current_tm = localtime(&current_time);
  tick_handler(current_tm, MINUTE_UNIT);

  //Setup tick time handler
  tick_timer_service_subscribe((MINUTE_UNIT), tick_handler);
  
}

static void window_unload(Window *window) {
}

static void init(void) {
  light_enable(true);
  window = window_create();
  //window_set_fullscreen(window, true);
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
      });
  window_stack_push(window, true);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
