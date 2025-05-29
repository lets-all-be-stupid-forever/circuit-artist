#include "colors.h"

Color GetLutColor(ColorLut c) {
  switch (c) {
    case COLOR_BTN0:
      return GetColor(0x6C6C53FF);  // darker
    case COLOR_BTN1:
      return GetColor(0xBBC29CFF);
    case COLOR_BTN2:
      return GetColor(0xF8FFCBFF);  // lighter
    case COLOR_ORANGE:
    case COLOR_BG0:
      return GetColor(0xBB830BFF);  // ORANGE
    case COLOR_BG1:
    case COLOR_DARKGRAY:
      return GetColor(0x13151AFF);  // DARK GRAY
    case COLOR_FC0:
      return WHITE;
    case COLOR_BTN_BG:
      return BLACK;
  }
}
