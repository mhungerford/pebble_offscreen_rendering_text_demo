#include <pebble.h>

/*
#define USE_FIXED_POINT 1

#ifdef USE_FIXED_POINT
  #include "math-sll.h"
#else
  #define CONST_PI M_PI
#endif
*/

#define FIREWORKS 4           // Number of fireworks
#define FIREWORK_PARTICLES 32  // Number of particles per firework

typedef struct FireworkStruct {
  float   x[FIREWORK_PARTICLES];
  float   y[FIREWORK_PARTICLES];
  float   xSpeed[FIREWORK_PARTICLES];
  float   ySpeed[FIREWORK_PARTICLES];

  int     framesUntilLaunch;
  float   particleSize;
  int     hasExploded;
  int     red, green, blue;
  float   alpha;
} FireworkStruct;

static FireworkStruct prv_fireworks[FIREWORKS];
static int screen_width = 0;
static int screen_height = 0;
static GRect screen_bounds;

const float Firework_GRAVITY = 0.08f;
const float Firework_baselineYSpeed = -2.5f;
const float Firework_maxYSpeed = -3.0f;

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

void Firework_Start(FireworkStruct* firework) {
  // Pick an initial x location and  random x/y speeds
  float xLoc = 10 + (rand() % (screen_width - 20));
  float xSpeedVal = -2 + (rand() / (float)RAND_MAX) * 4.0f;
  float ySpeedVal = Firework_baselineYSpeed + ((float)rand() / (float)RAND_MAX) * Firework_maxYSpeed;

  // Set initial x/y location and speeds
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
  {
    firework->x[loop] = xLoc;
    firework->y[loop] = screen_height + 10.0f; // Push the particle location down off the bottom of the screen
    firework->xSpeed[loop] = xSpeedVal;
    firework->ySpeed[loop] = ySpeedVal;
  }

  firework->red   = 85 + (rand() % 170);
  firework->green = 85 + (rand() % 170);
  firework->blue  = 85 + (rand() % 170);
  firework->alpha = 1.0f;

  firework->framesUntilLaunch = ((int)rand() % 400);

  firework->hasExploded = false;
}

void Firework_Move(FireworkStruct* firework) {
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
  {
    // Once the firework is ready to launch start moving the particles
    if (firework->framesUntilLaunch <= 0)
    {
      firework->x[loop] += firework->xSpeed[loop];
      firework->y[loop] += firework->ySpeed[loop];
      firework->ySpeed[loop] += Firework_GRAVITY;
    }
  }
  firework->framesUntilLaunch--;

  // Once a fireworks speed turns positive (i.e. at top of arc) - blow it up!
  if (firework->ySpeed[0] > 0.0f)
  {
    for (int loop2 = 0; loop2 < FIREWORK_PARTICLES; loop2++)
    {
      // Set a random x and y speed beteen -4 and + 4
      firework->xSpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
      firework->ySpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
    }

    firework->hasExploded = true;
  }
}

void Firework_Explode(FireworkStruct* firework) {
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++) {
    // Dampen the horizontal speed by 1% per frame
    firework->xSpeed[loop] *= 0.99f;

    // Move the particle
    firework->x[loop] += firework->xSpeed[loop];
    firework->y[loop] += firework->ySpeed[loop];

    // Apply gravity to the particle's speed
    firework->ySpeed[loop] += Firework_GRAVITY;
  }

  // Fade out the particles (alpha is stored per firework, not per particle)
  if (firework->alpha > 0.0f)
  {
    firework->alpha -= 0.01f;
  }
  else // Once the alpha hits zero reset the firework
  {
    Firework_Start(firework);
  }
}

void Firework_Initialize(int width, int height) {
  screen_width = width;
  screen_height = height;
  screen_bounds = (GRect) {{0, 0}, {width, height}};

  for (int i = 0; i < FIREWORKS; i++) {
    Firework_Start(&prv_fireworks[i]);
  }
}

void Firework_Update(GContext *ctx, int width, int height) {
  static int darken_count = 0;
  if (darken_count++ >= 10) {
    darken_count = 0;
    graphics_darken(ctx);
  }
  // Draw fireworks
  for (int i = 0; i < FIREWORKS; i++) {
    FireworkStruct* firework = &prv_fireworks[i];
    GColor color;
    if (firework->hasExploded) {
      color = GColorFromRGB(
          (int)(firework->red * firework->alpha),
          (int)(firework->green * firework->alpha),
          (int)(firework->blue * firework->alpha));
    } else {
      color = GColorFromRGB(255, 255, 0);
    }

    for (int p = 0; p < FIREWORK_PARTICLES; p++) {
      graphics_context_set_stroke_color(ctx, color);
      if (firework->x[p] < screen_width && firework->y[p] < screen_height) {
        graphics_draw_pixel_color(ctx, (GPoint){firework->x[p], firework->y[p]}, color);
        graphics_draw_pixel_color(ctx, (GPoint){firework->x[p] + 1, firework->y[p]}, color);
        graphics_draw_pixel_color(ctx, (GPoint){firework->x[p] + 1, firework->y[p] + 1}, color);
        graphics_draw_pixel_color(ctx, (GPoint){firework->x[p], firework->y[p] + 1}, color);
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


