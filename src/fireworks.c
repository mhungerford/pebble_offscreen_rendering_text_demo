#include <pebble.h>


#define USE_FIXED_POINT 1

#ifdef USE_FIXED_POINT
  #include "math-sll.h"
#else
  #define CONST_PI M_PI
#endif

#define FIREWORKS 6           // Number of fireworks
#define FIREWORK_PARTICLES 32  // Number of particles per firework

typedef struct FireworkStruct {
  sll   x[FIREWORK_PARTICLES];
  sll   y[FIREWORK_PARTICLES];
  sll   xSpeed[FIREWORK_PARTICLES];
  sll   ySpeed[FIREWORK_PARTICLES];

  int   framesUntilLaunch;
  int   hasExploded;
  int   red, green, blue;
  sll   alpha;
} FireworkStruct;

static FireworkStruct prv_fireworks[FIREWORKS];
static int screen_width = 0;
static int screen_height = 0;

static sll Firework_GRAVITY = 0;
static sll Firework_baselineYSpeed = 0;
static sll Firework_maxYSpeed = 0;
static sll Firework_xDampen = 0;
static sll Firework_aDampen = 0;

typedef struct MyGBitmap {
  void *addr;
  uint16_t row_size_bytes;
  uint16_t info_flags;
  GRect bounds;
} MyGBitmap;

typedef struct MyGContext {
  MyGBitmap dest_bitmap;
} MyGContext;


void graphics_draw_pixel_color(GContext* ctx, GPoint point, GColor color) {
  MyGBitmap *bitmap = &((MyGContext*)ctx)->dest_bitmap;
  if (grect_contains_point(&bitmap->bounds, &point)) {
    uint8_t *line = ((uint8_t*)bitmap->addr) + (bitmap->row_size_bytes * point.y);
    line[point.x] = color.argb;
  }
}

void graphics_darken(GContext* ctx) {
  MyGBitmap *bitmap = &((MyGContext*)ctx)->dest_bitmap;
  static int count = 0;
  count = (count + 1) % 4;

  for (int y = bitmap->bounds.origin.y; y < bitmap->bounds.origin.y + bitmap->bounds.size.h; y++) {
    for (int x = bitmap->bounds.origin.x; x < bitmap->bounds.origin.x + bitmap->bounds.size.w; x++) {
      if ((x + y) % 4 == count) {
        uint8_t *line = ((uint8_t*)bitmap->addr) + (bitmap->row_size_bytes * y);
        GColor pixel = (GColor)line[x];
        if (pixel.a > 0) {
          pixel.a--;
        } else if (pixel.g >= pixel.r && pixel.g > 0) {
          pixel.g--;
        } else if (pixel.r >= pixel.b && pixel.r > 0) {
          pixel.r--;
        } else if (pixel.b > 0) {
          pixel.b--;
        }
        line[x] = pixel.argb;
      }
    }
  }
}

sll rand_sll(sll max_val) {
  sll max_scale = int2sll(1000);
  int max_val_int = sll2int(sllmul(max_val, max_scale));
  return slldiv(int2sll(rand() % max_val_int), max_scale);
}

void Firework_Start(FireworkStruct* firework) {
  // Pick an initial x location and  random x/y speeds
  sll xLoc = int2sll(10 + (rand() % (screen_width - 20)));
  sll yLoc = int2sll(screen_height + 10); // start off the bottom of the screen
  sll xSpeedVal = slladd(int2sll(-2), rand_sll(CONST_4));
  sll ySpeedVal = sllsub(Firework_baselineYSpeed, rand_sll(Firework_maxYSpeed));

  // Set initial x/y location and speeds
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++) {
    firework->x[loop] = xLoc;
    firework->y[loop] = yLoc;
    firework->xSpeed[loop] = xSpeedVal;
    firework->ySpeed[loop] = ySpeedVal;
  }

  firework->red   = 85 + (rand() % 170);
  firework->green = 85 + (rand() % 170);
  firework->blue  = 85 + (rand() % 170);
  firework->alpha = CONST_1;

  firework->framesUntilLaunch = (rand() % 400);

  firework->hasExploded = false;
}

void Firework_Move(FireworkStruct* firework) {
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++) {
    // Once the firework is ready to launch start moving the particles
    if (firework->framesUntilLaunch <= 0) {
      firework->x[loop] = slladd(firework->x[loop], firework->xSpeed[loop]);
      firework->y[loop] = slladd(firework->y[loop], firework->ySpeed[loop]);
      firework->ySpeed[loop] = slladd(firework->ySpeed[loop], Firework_GRAVITY);
    }
  }
  firework->framesUntilLaunch--;

  // Once a fireworks speed turns positive (i.e. at top of arc) - blow it up!
  if (firework->ySpeed[0] > CONST_0) {
    for (int loop2 = 0; loop2 < FIREWORK_PARTICLES; loop2++) {
      // Set a random x and y speed beteen -4 and + 4
      firework->xSpeed[loop2] = slladd(int2sll(-4), rand_sll(CONST_8));
      firework->ySpeed[loop2] = slladd(int2sll(-4), rand_sll(CONST_8));
    }

    firework->hasExploded = true;
  }
}

void Firework_Explode(FireworkStruct* firework) {
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++) {
    // Dampen the horizontal speed by 1% per frame
    firework->xSpeed[loop] = sllmul(firework->xSpeed[loop], Firework_xDampen);

    // Move the particle
    firework->x[loop] = slladd(firework->x[loop], firework->xSpeed[loop]);
    firework->y[loop] = slladd(firework->y[loop], firework->ySpeed[loop]);

    // Apply gravity to the particle's speed
    firework->ySpeed[loop] = slladd(firework->ySpeed[loop], Firework_GRAVITY);
  }

  // Fade out the particles (alpha is stored per firework, not per particle)
  if (firework->alpha > CONST_0) {
    firework->alpha = sllsub(firework->alpha, Firework_aDampen);
  } else { // Once the alpha hits zero reset the firework
    Firework_Start(firework);
  }
}

void Firework_Initialize(int width, int height) {
  Firework_GRAVITY = slldiv(CONST_8, int2sll(100));
  Firework_baselineYSpeed = slldiv(int2sll(-25), CONST_10);
  Firework_maxYSpeed = CONST_3; // This gets negated later
  Firework_xDampen = slldiv(int2sll(99), int2sll(100));
  Firework_aDampen = slldiv(CONST_1, int2sll(100));
  
  screen_width = width;
  screen_height = height;

  for (int i = 0; i < FIREWORKS; i++) {
    Firework_Start(&prv_fireworks[i]);
  }
}

void Firework_Update(GContext *ctx, int width, int height) {
  static int darken_count = 0;
  if (darken_count++ >= 4) {
    darken_count = 0;
    graphics_darken(ctx);
  }
  // Draw fireworks
  for (int i = 0; i < FIREWORKS; i++) {
    FireworkStruct* firework = &prv_fireworks[i];
    GColor color;
    if (firework->hasExploded) {
      color = GColorFromRGB(
          sll2int(sllmul(int2sll(firework->red), firework->alpha)),
          sll2int(sllmul(int2sll(firework->green), firework->alpha)),
          sll2int(sllmul(int2sll(firework->blue), firework->alpha)));
    } else {
      color = GColorFromRGB(255, 255, 0);
    }

    for (int p = 0; p < FIREWORK_PARTICLES; p++) {
      graphics_context_set_stroke_color(ctx, color);
      int xpos = sll2int(firework->x[p]);
      int ypos = sll2int(firework->y[p]);
      if (xpos < screen_width && ypos < screen_height) {
        graphics_draw_pixel_color(ctx, (GPoint){xpos, ypos}, color);
        graphics_draw_pixel_color(ctx, (GPoint){xpos + 1, ypos}, color);
        graphics_draw_pixel_color(ctx, (GPoint){xpos + 1, ypos + 1}, color);
        graphics_draw_pixel_color(ctx, (GPoint){xpos, ypos + 1}, color);
      }
    }
  }

  for (int i = 0; i < FIREWORKS; i++) {
    if (!prv_fireworks[i].hasExploded)
      Firework_Move(&prv_fireworks[i]);
    else
      Firework_Explode(&prv_fireworks[i]);
  }
}


