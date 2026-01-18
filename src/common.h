#ifndef CA_COMMON_H
#define CA_COMMON_H
#include <inttypes.h>
#include <raylib.h>
#include <rlgl.h>
#include <string.h>

#define GL_INT 0x1404
#define M_PI 3.141592653589793

#define PROGRESS_FILE "../save_1_1.json"
#define STAMPS_FILE "../blueprints_1_1.json"

#define SIM_MAX_QUEUE_DELAY (8000)
#define SIM_MAX_WIRE_DELAY (6000)

#include "stdbool.h"
#define CA_VERSION "v1.1.0"

#define MAX_LAYERS 3

#define COLOR_EQ(u, v) \
  (((u).r == (v).r) && ((u).g == (v).g) && ((u).b == (v).b) && ((u).a == (v).a))

// Tool icons
#define rect_brush ((Rectangle){16 * 0, 16 * 0, 13, 13})
#define rect_line ((Rectangle){16 * 2, 16 * 0, 13, 13})
#define rect_marquee ((Rectangle){16 * 1, 16 * 0, 13, 13})
#define rect_text ((Rectangle){16 * 3, 16 * 0, 13, 13})
#define rect_bucket ((Rectangle){16 * 12, 16 * 0, 13, 13})
#define rect_picker ((Rectangle){16 * 13, 16 * 0, 13, 13})

// Play/Stop icons
#define rect_start ((Rectangle){16 * 0, 16 * 1, 13, 13})
#define rect_pause ((Rectangle){16 * 1, 16 * 1, 13, 13})
#define rect_stop ((Rectangle){16 * 2, 16 * 1, 13, 13})
#define rect_rewind ((Rectangle){16 * 3, 16 * 1, 13, 13})
#define rect_forward ((Rectangle){16 * 4, 16 * 1, 13, 13})

// Logo of CircuitArtist (for About page)
#define rect_logo ((Rectangle){160, 80, 158, 87})
#define rect_logo_version ((Rectangle){160, 80, 158, 67})

// top bar icons
#define rect_new ((Rectangle){176, 16, 13, 13})
#define rect_open ((Rectangle){192, 16, 13, 13})
#define rect_save ((Rectangle){208, 16, 13, 13})
#define rect_saveas ((Rectangle){224, 16, 13, 13})
#define rect_info ((Rectangle){432, 16, 13, 13})
#define rect_exit ((Rectangle){240, 16, 13, 13})
#define rect_editpage ((Rectangle){48, 32, 13, 13})
#define rect_blueprint ((Rectangle){48, 48, 13, 13})
#define rect_blueprint_add ((Rectangle){48 + 16, 48, 13, 13})

#define rect_eye_on ((Rectangle){272, 16, 13, 13})
#define rect_eye_off ((Rectangle){256, 16, 13, 13})
// Selection open/save
#define rect_sel_open ((Rectangle){464, 16, 13, 13})
#define rect_sel_save ((Rectangle){464, 0, 13, 13})
#define rect_inspect_wire ((Rectangle){80, 32, 13, 13})

// Clock speed icons
#define rect_hz0 ((Rectangle){352, 16, 13, 13})
#define rect_hz1 ((Rectangle){352, 0, 13, 13})
#define rect_hz4 ((Rectangle){368, 0, 13, 13})
#define rect_hz16 ((Rectangle){384, 0, 13, 13})
#define rect_hz64 ((Rectangle){368, 16, 13, 13})
#define rect_hz1k ((Rectangle){384, 16, 13, 13})
#define rect_medal ((Rectangle){560, 0, 13, 13})

// Simulation mode
#define rect_simu_over ((Rectangle){512, 0, 13, 13})
#define rect_simu_step ((Rectangle){528, 0, 13, 13})

// Marquee sub-tools icons
#define rect_rot ((Rectangle){160, 0, 13, 13})
#define rect_fliph ((Rectangle){112, 0 * 0, 13, 13})
#define rect_flipv ((Rectangle){128, 0 * 0, 13, 13})
#define rect_fill ((Rectangle){144, 0 * 0, 13, 13})
#define rect_line_sep ((Rectangle){480, 16, 13, 13})
#define rect_line_sep_r ((Rectangle){496, 16, 13, 13})

// minimap icon
#define rect_map ((Rectangle){448, 0, 13, 13})
#define rect_flag ((Rectangle){0, 128, 13, 13})
#define rect_success ((Rectangle){16, 128, 48, 48})

// mouse icons
#define rect_mouse_switch ((Rectangle){144, 32, 32, 32})
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
#define rect_mouse_clock \
  ((Rectangle){.x = 528, .y = 32, .width = 32, .height = 32})

// Background pattern
// #define rect_bg_pattern ((Rectangle){800, 0, 12, 12})
#define rect_bg_pattern ((Rectangle){736, 48, 64, 64})

// Some unused icons
#define rect_bulbon ((Rectangle){336, 16, 13, 13})
#define rect_clock ((Rectangle){336, 0, 13, 13})
#define rect_chip ((Rectangle){224, 0, 13, 13})
#define rect_help ((Rectangle){432, 0, 13, 13})
#define rect_stairs ((Rectangle){240, 16 * 0, 13, 13})
#define rect_book ((Rectangle){448, 16, 13, 13})
#define rect_msg ((Rectangle){320, 16 * 1, 13, 13})
#define rect_warning ((Rectangle){320, 16 * 1, 13, 13})
#define rect_tree ((Rectangle){320, 0 * 0, 13, 13})
#define rect_valid ((Rectangle){4, 81, 33, 33})
#define rect_cancel ((Rectangle){416, 112, 33, 33})
#define rect_locked ((Rectangle){39, 81, 33, 33})
#define rect_page ((Rectangle){256, 0, 13, 13})
#define rect_fullscreen ((Rectangle){256, 0, 13, 13})
#define rect_thumbsup ((Rectangle){0, 128, 48, 48})
#define rect_thumbsup2 ((Rectangle){0, 176, 48, 64})
#define rect_normalscreen ((Rectangle){272, 0, 13, 13})
#define rect_img ((Rectangle){288, 16, 13, 13})
#define rect_monitor ((Rectangle){304, 16, 13, 13})
#define rect_switch_pressed ((Rectangle){384, 0, 20, 24})
#define rect_switch_disabled ((Rectangle){448, 0, 20, 24})
#define rect_color ((Rectangle){272, 448, 16 * 4, 16 * 3})
#define rect_progress_incomplete ((Rectangle){416, 448, 33, 33})
#define rect_progress_complete ((Rectangle){464, 448, 33, 33})
#define rect_search ((Rectangle){304, 0, 13, 13})

#define rect_rabbit ((Rectangle){512, 16, 13, 13})
#define rect_snail ((Rectangle){528, 16, 13, 13})

typedef uint8_t u8;
typedef int8_t i8;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef Vector2 v2;
typedef Vector3 v3;
typedef Rectangle Rec;
typedef RenderTexture2D RTex2D;
typedef Texture2D Tex2D;

// Vector2, 2 components
typedef struct {
  int x;  // Vector x component
  int y;  // Vector y component
} Vector2Int;
typedef Vector2Int v2i;

// Rectangle with int coordinates.
typedef struct RectangleInt {
  int x;       // Rectangle top-left corner position x
  int y;       // Rectangle top-left corner position y
  int width;   // Rectangle width
  int height;  // Rectangle height
} RectangleInt;
typedef RectangleInt RecI;

/** 2D Camera for the paint canvas.
 *
 * The idea is that "off" is the screen pixel offset of the canvas origin (ie,
 * the top-left corner pixel), then the actual canvas is drawn such that each
 * canvas pixel will match "sp" screen pixels.
 */
typedef struct {
  v2i off;  /* Shift of the image on screen in screen pixels */
  float sp; /* Screen pixels for each canvas pixel */
} Cam2D;

// Simple descriptor of a sprite.
typedef struct {
  Texture2D tex;
  Rectangle region;
} sprite_t;

static inline void find_idx(int s, int w, int idx, int* l, int* y, int* x) {
  if (idx < 0) {
    idx = -idx - 1;
  }
  *l = idx / s;
  idx -= *l * s;
  *y = idx / w;
  *x = idx % w;
}

typedef struct {
  float r_per_w[MAX_LAYERS];
  float c_per_w[MAX_LAYERS];
  float fixed_gate_delay;
  float l_socket; /* Equivalent wire length for nand gate capacitance input */
  double c_gate;
  double r_gate;
  double vdd;
  double lone_pulse_energy; /* Pulse energy for LONE wires */
} DistSpec;

#endif
