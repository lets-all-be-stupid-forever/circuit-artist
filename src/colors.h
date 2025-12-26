#ifndef CA_COLORS_H
#define CA_COLORS_H
#include "raylib.h"

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

Color get_lut_color(ColorLut c);

#endif
