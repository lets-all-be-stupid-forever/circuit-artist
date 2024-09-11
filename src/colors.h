#ifndef COLORS_H
#define COLORS_H
#include "defs.h"

// Palette for the UI colors.
typedef enum {
  COLOR_BTN0,
  COLOR_BTN1,
  COLOR_BTN2,
  COLOR_BTN_BG,
  COLOR_BG0,
  COLOR_BG1,
  COLOR_FC0,
  COLOR_DARKGRAY,
  COLOR_ORANGE,
} ColorLut;

// Returns the raylib color for a given color name.
Color GetLutColor(ColorLut c);

// From RGB color to 0-255 gray value.
static inline int ColorToGray(Color c)
{
  float r = c.r;
  float g = c.g;
  float b = c.b;
  int out = 0.299f * r + 0.587f * g + 0.114f * b;
  if (out < 0) out = 0;
  if (out > 255) out = 255;
  return out;
}

#endif
