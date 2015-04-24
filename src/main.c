#include <pebble.h>

#define USE_FIXED_POINT 1

#ifdef USE_FIXED_POINT
  #include "math-sll.h"
#else
  #define CONST_PI M_PI
#endif


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
}

static void window_unload(Window *window) {
}

static void init(void) {
  window = window_create();
  window_set_fullscreen(window, true);
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
