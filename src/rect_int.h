#ifndef RECT_INT_H
#define RECT_INT_H
#include "stdbool.h"

// Rectangle with int coordinates.
typedef struct {
  int x;       // Rectangle top-left corner position x
  int y;       // Rectangle top-left corner position y
  int width;   // Rectangle width
  int height;  // Rectangle height
} RectangleInt;

// Checks if the rectangle is empty. (ie area == 0)
bool IsRecIntEmpty(RectangleInt r);

// Checks collision between int rectangles (similar to CheckRectCollision).
bool CheckRecIntCollision(RectangleInt a, RectangleInt b);

// Computes the intersection of two rectangles.
RectangleInt GetCollisionRecInt(RectangleInt a, RectangleInt b);

#endif
