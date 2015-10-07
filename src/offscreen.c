#include <pebble.h>

void offscreen_draw_pixel_color(GContext *ctx, GPoint point, GColor color) {
  /*
  MyGBitmap *bitmap = (GBitmap*)ctx;
  if (grect_contains_point(&bitmap->bounds, &point)) {
    uint8_t *line = ((uint8_t*)bitmap->addr) + (bitmap->row_size_bytes * point.y);
    line[point.x] = color.argb;
  }
  */
}

void offscreen_make_transparent(GContext *ctx) {
  GBitmap *bitmap = (GBitmap*)ctx;
  GRect bounds = gbitmap_get_bounds(bitmap);
  for (int y = bounds.origin.y; y < bounds.origin.y + bounds.size.h; y++) {
    GBitmapDataRowInfo row_info = gbitmap_get_data_row_info(bitmap, y);
    for (int x = row_info.min_x; x < row_info.max_x; x++) {
      GColor *pixel = (GColor*)&row_info.data[x];
      if (pixel->a == 0x3) {
        //pixel->a = ((x%2 + y%2) % 2) ? 0x2 : 0x1;
        pixel->a = 0x2;
      }
    }
  }
}
