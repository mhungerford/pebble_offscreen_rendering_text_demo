#include <pebble.h>

// fireworks draws to the image buffer in a gbitmap so that it can use the alpha channel
// in an alternative way.
// Every 5 frames, the alpha channel is evaluated, lowered by 1 value, and any pixel
// with an alpha of 0 is slowly darkened (lowering the brightest channel)
// To make the darkening less abrupt, alternate pixels are evaluated and darkened.
//
// Fireworks particle effects based on :
// http://r3dux.org/2010/10/how-to-create-a-simple-fireworks-effect-in-opengl-and-sdl/

#define USE_FIXED_POINT 1

#ifdef USE_FIXED_POINT
  #include "math-sll.h"
#else
  #define CONST_PI M_PI
#endif

#define FIREWORKS 4           // Number of fireworks
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

void graphics_draw_pixel_color(GBitmap *bitmap, GPoint point, GColor color) {
  uint16_t row_size_bytes = gbitmap_get_bytes_per_row(bitmap);
  uint8_t *imagebuffer = gbitmap_get_data(bitmap);
  GRect bounds = gbitmap_get_bounds(bitmap);
  if (grect_contains_point(&bounds, &point)) {
    uint8_t *line = imagebuffer + (row_size_bytes * point.y);
    line[point.x] = color.argb;
  }
}

void graphics_darken(GBitmap *bitmap) {
  static int count = 0;
  count = (count + 1) % 4;
  uint16_t row_size_bytes = gbitmap_get_bytes_per_row(bitmap);
  uint8_t *imagebuffer = gbitmap_get_data(bitmap);
  GRect bounds = gbitmap_get_bounds(bitmap);

  for (int y = bounds.origin.y; y < bounds.origin.y + bounds.size.h; y++) {
    for (int x = bounds.origin.x; x < bounds.origin.x + bounds.size.w; x++) {
      if ((x + y) % 4 == count) {
        uint8_t *line = imagebuffer + (row_size_bytes * y);
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
  return slldiv(int2sll(rand() % (max_val_int + 1)), max_scale);
}

void Firework_Start(FireworkStruct* firework) {
  // Pick an initial x location and  random x/y speeds
  sll xLoc = int2sll(10 + (rand() % (screen_width - 20)));
  sll yLoc = int2sll(screen_height); // start at the bottom of the screen
  sll xSpeedVal = slladd(int2sll(-2), rand_sll(CONST_4));
  sll ySpeedVal = sllsub(Firework_baselineYSpeed, rand_sll(Firework_maxYSpeed));

  // Set initial x/y location and speeds
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++) {
    firework->x[loop] = xLoc;
    firework->y[loop] = yLoc;
    firework->xSpeed[loop] = xSpeedVal;
    firework->ySpeed[loop] = ySpeedVal;
  }

  // Max out at 170 for more vibrant colors
  firework->red   = ((rand() % 4) * 85);
  firework->green = ((rand() % 4) * 85);
  firework->blue  = ((rand() % 4) * 85);
  
  // Instead of white, black or gray, do GColorFolly
  if (firework->red == firework->green  && firework->green && firework->blue) {
    firework->red = 255;
    firework->blue = 85;
  } else if (firework->red == 0 && firework->green == 0 && firework->blue == 85) {
    // Oxford blue too dark, do Cyan
    firework->green = 255;
    firework->blue =  255;
  }

  firework->alpha = CONST_1;

  firework->framesUntilLaunch = (rand() % 100);

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
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++) {
      // Set a random x and y speed beteen -4 and + 4
      firework->xSpeed[loop] = slladd(int2sll(-4), rand_sll(CONST_8));
      firework->ySpeed[loop] = slladd(int2sll(-4), rand_sll(CONST_8));
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
    if (firework->alpha < CONST_0) {
      firework->alpha = CONST_0;
    }
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

void Firework_Update(GBitmap *bitmap) {
  static int darken_count = 0;
  if (darken_count++ >= 4) {
    darken_count = 0;
    graphics_darken(bitmap);
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

      for (int p = 0; p < FIREWORK_PARTICLES; p++) {
        int xpos = sll2int(firework->x[p]);
        int ypos = sll2int(firework->y[p]);
        graphics_draw_pixel_color(bitmap, (GPoint){xpos, ypos}, color);
        graphics_draw_pixel_color(bitmap, (GPoint){xpos + 1, ypos}, color);
        graphics_draw_pixel_color(bitmap, (GPoint){xpos + 1, ypos + 1}, color);
        graphics_draw_pixel_color(bitmap, (GPoint){xpos, ypos + 1}, color);
      }

      Firework_Explode(&prv_fireworks[i]);
    } else {
      color = GColorFromRGB(255, 255, 0);
      int xpos = sll2int(firework->x[0]);
      int ypos = sll2int(firework->y[0]);
      graphics_draw_pixel_color(bitmap, (GPoint){xpos, ypos}, color);
      graphics_draw_pixel_color(bitmap, (GPoint){xpos + 1, ypos}, color);
      graphics_draw_pixel_color(bitmap, (GPoint){xpos + 1, ypos + 1}, color);
      graphics_draw_pixel_color(bitmap, (GPoint){xpos, ypos + 1}, color);

      Firework_Move(&prv_fireworks[i]);
    }
  }
}

