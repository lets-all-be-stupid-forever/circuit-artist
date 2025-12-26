#include "rectint.h"

// Checks if the rectangle is empty. (ie area == 0)
bool rect_int_is_empty(RectangleInt r) { return r.width == 0 || r.height == 0; }

// Checks collision between int rectangles (similar to CheckRectCollision).
bool rect_int_check_collision(RectangleInt a, RectangleInt b) {
  int r1x = a.x;
  int r1y = a.y;
  int r1w = a.width;
  int r1h = a.height;
  int r2x = b.x;
  int r2y = b.y;
  int r2w = b.width;
  int r2h = b.height;
  if (r1x + r1w >= r2x &&  // r1 right edge past r2 left
      r1x <= r2x + r2w &&  // r1 left edge past r2 right
      r1y + r1h >= r2y &&  // r1 top edge past r2 bottom
      r1y <= r2y + r2h) {  // r1 bottom edge past r2 top
    return true;
  }
  return false;
}

// Computes the intersection of two rectangles.
RectangleInt rect_int_get_collision(RectangleInt a, RectangleInt b) {
  // adapted from raylib
  RectangleInt overlap = {0};

  int left = (a.x > b.x) ? a.x : b.x;
  int right1 = a.x + a.width;
  int right2 = b.x + b.width;
  int right = (right1 < right2) ? right1 : right2;
  int top = (a.y > b.y) ? a.y : b.y;
  int bottom1 = a.y + a.height;
  int bottom2 = b.y + b.height;
  int bottom = (bottom1 < bottom2) ? bottom1 : bottom2;

  if ((left < right) && (top < bottom)) {
    overlap.x = left;
    overlap.y = top;
    overlap.width = right - left;
    overlap.height = bottom - top;
  }

  return overlap;
}
