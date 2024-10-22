#ifndef DEFS_H
#define DEFS_H
// poor CLion can't tell this is used by everyone who includes defs.h
// ReSharper disable once CppUnusedIncludeDirective
#include <raylib.h>

#define COLOR_EQ(u, v) \
  (((u).r == (v).r) && ((u).g == (v).g) && ((u).b == (v).b) && ((u).a == (v).a))

#ifndef WEB
// Maximum image size is 8k.
// For images bigger than that, a dedicated exporter/reader is necessary
// (stb_image blocks at 8k by default)
#define MAX_IMG_SIZE (8 * 1024)
#else
#define MAX_IMG_SIZE 512
#endif

#if defined(__linux__) || defined(__APPLE__)
#define CA_API
#elif _WIN32
#define CA_API __declspec(dllexport)
#endif

// ------------------------------------------------------
// List of rects of icons in the assets/sprite4.png atlas.
// ------------------------------------------------------

// Tool icons
#define rect_brush ((Rectangle){16 * 0, 16 * 0, 13, 13})
#define rect_line ((Rectangle){16 * 2, 16 * 0, 13, 13})
#define rect_marquee ((Rectangle){16 * 1, 16 * 0, 13, 13})
#define rect_text ((Rectangle){16 * 3, 16 * 0, 13, 13})
#define rect_bucket ((Rectangle){16 * 12, 16 * 0, 13, 13})
#define rect_picker ((Rectangle){16 * 13, 16 * 0, 13, 13})

// Play/Stop icons
#define rect_start ((Rectangle){16 * 0, 16 * 1, 13, 13})
#define rect_stop ((Rectangle){16 * 2, 16 * 1, 13, 13})

// Icon of the sandbox challenge for the WEB version, which doesn't have lua.
#define rect_sandbox ((Rectangle){368, 80, 33, 33})

// Logo of CircuitArtist (for About page)
#define rect_logo ((Rectangle){160, 80, 158, 67})

// top bar icons
#define rect_new ((Rectangle){176, 16, 13, 13})
#define rect_open ((Rectangle){192, 16, 13, 13})
#define rect_save ((Rectangle){208, 16, 13, 13})
#define rect_saveas ((Rectangle){224, 16, 13, 13})
#define rect_info ((Rectangle){432, 16, 13, 13})
#define rect_exit ((Rectangle){240, 16, 13, 13})

// Selection open/save
#define rect_sel_open ((Rectangle){464, 16, 13, 13})
#define rect_sel_save ((Rectangle){464, 0, 13, 13})

// Clock speed icons
#define rect_hz0 ((Rectangle){352, 16, 13, 13})
#define rect_hz1 ((Rectangle){352, 0, 13, 13})
#define rect_hz4 ((Rectangle){368, 0, 13, 13})
#define rect_hz16 ((Rectangle){384, 0, 13, 13})
#define rect_hz64 ((Rectangle){368, 16, 13, 13})
#define rect_hz1k ((Rectangle){384, 16, 13, 13})

// Marquee sub-tools icons
#define rect_rot ((Rectangle){160, 0, 13, 13})
#define rect_fliph ((Rectangle){112, 0 * 0, 13, 13})
#define rect_flipv ((Rectangle){128, 0 * 0, 13, 13})
#define rect_fill ((Rectangle){144, 0 * 0, 13, 13})

// minimap icon
#define rect_map ((Rectangle){448, 0, 13, 13})

// mouse icons
#define rect_mouse_arrow \
  ((Rectangle){.x = 0, .y = 32, .width = 32, .height = 32})
#define rect_mouse_pen_alone \
  ((Rectangle){.x = 176, .y = 32, .width = 32, .height = 32})
#define rect_mouse_cross2 \
  ((Rectangle){.x = 208, .y = 32, .width = 32, .height = 32})
#define rect_mouse_movecross \
  ((Rectangle){.x = 336, .y = 32, .width = 32, .height = 32})
#define rect_mouse_pen \
  ((Rectangle){.x = 240, .y = 32, .width = 32, .height = 32})
#define rect_mouse_bucket \
  ((Rectangle){.x = 272, .y = 32, .width = 32, .height = 32})
#define rect_mouse_picker \
  ((Rectangle){.x = 304, .y = 32, .width = 32, .height = 32})
#define rect_mouse_resize \
  ((Rectangle){.x = 368, .y = 32, .width = 32, .height = 32})
#define rect_mouse_pointer \
  ((Rectangle){.x = 432, .y = 32, .width = 32, .height = 32})

// Background pattern
#define rect_bg_pattern ((Rectangle){800, 0, 12, 12})

// Some unused icons
#define rect_clock ((Rectangle){336, 0, 13, 13})
#define rect_chip ((Rectangle){224, 0, 13, 13})
#define rect_help ((Rectangle){432, 0, 13, 13})
#define rect_stairs ((Rectangle){240, 16 * 0, 13, 13})
#define rect_book ((Rectangle){320, 448, 48, 64})
#define rect_msg ((Rectangle){320, 16 * 1, 13, 13})
#define rect_tree ((Rectangle){320, 0 * 0, 13, 13})
#define rect_valid ((Rectangle){4, 81, 33, 33})
#define rect_cancel ((Rectangle){416, 112, 33, 33})
#define rect_locked ((Rectangle){39, 81, 33, 33})
#define rect_fullscreen ((Rectangle){256, 0, 13, 13})
#define rect_thumbsup ((Rectangle){0, 128, 48, 48})
#define rect_thumbsup2 ((Rectangle){0, 176, 48, 64})
#define rect_check2 ((Rectangle){65, 128, 15, 13})
#define rect_normalscreen ((Rectangle){272, 0, 13, 13})
#define rect_img ((Rectangle){288, 16, 13, 13})
#define rect_monitor ((Rectangle){304, 16, 13, 13})
#define rect_switch_pressed ((Rectangle){384, 0, 20, 24})
#define rect_switch_disabled ((Rectangle){448, 0, 20, 24})
#define rect_color ((Rectangle){272, 448, 16 * 4, 16 * 3})
#define rect_progress_incomplete ((Rectangle){416, 448, 33, 33})
#define rect_progress_complete ((Rectangle){464, 448, 33, 33})
#define rect_search ((Rectangle){304, 0, 13, 13})

// Vector2, 2 components
typedef struct Vector2Int {
  int x;  // Vector x component
  int y;  // Vector y component
} Vector2Int;

static inline int MinInt(int a, int b) { return a < b ? a : b; }
static inline int MaxInt(int a, int b) { return a > b ? a : b; }

#endif
