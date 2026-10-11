#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <i18n.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cam2d.h"
#include "font.h"
#include "img.h"
#include "layout.h"
#include "lua_level.h"
#include "raylib.h"
#include "shaders.h"
#include "sim.h"
#include "sound.h"
#include "stdio.h"
#include "tex.h"
#include "ui.h"
#include "uifont.h"
#include "utils.h"
#include "widgets.h"

typedef enum {
  MODE_EMPTY,
  MODE_LOAD_IMG_PENDING,
  MODE_LOAD_IMG_ERROR,
  MODE_SIM_ERROR,
  MODE_SIMU,
} PlayerMode;

/* Search radius (image pixels) when clicking near a wire to toggle it. */
#define TOGGLE_SEARCH_RADIUS 5
/* Keyboard pan speed, screen pixels per second (same as the paint canvas). */
#define PAN_SPEED 600.0
/* Time-travel clock sensitivity: ticks per degree of rotation, scaled by tps.
 */
#define CLOCK_SENSITIVITY 0.003
#define PAD_TOP 100
#define PAD_BOT 50
#define PAD_LEFT 80
#define PAD_RIGHT 20

// States:
//   Empty
//   Ready
//   ErrorLoad
//   Running
//   Error
//
static struct {
  LevelAPI api;
  Layout* layout;
  RenderTexture2D t_buffer[3];
  Image buffer[3];
  int nl;
  v2i img_size;
  Sim sim;
  HSim hsim;
  Cam2D cam;
  int cam_idx;
  RecI viewport; /* Region of the target the circuit is drawn on (all of it) */
  PlayerMode mode;
  double simu_target_steps;
  int pix_toggle;
  bool sound_on;
  int hover_pix; /* Wire pixel under the cursor, -1 if none */
  bool paused;
  bool forward_pressed;
  bool rewind_pressed;
  /* Keyboard pan accumulators, so sub-pixel movement isn't lost at low fps */
  double pan_acc_x, pan_acc_y;
  /* Time-travel clock (right mouse button held) */
  bool time_open;
  double time_ref;
  v2 time_pos_ref;
  v2 time_c;
  Listbox lb;

  /* Load requested from outside the frame loop (JS), consumed at the top of
   * the next frame. See ca_load_image() / ca_load_png(). Exactly one of the
   * two is set: pending_png wins when non-NULL. */
  bool pending_load;
  char pending_path[512];
  unsigned char* pending_png; /* owned here, freed once decoded */
  int pending_png_len;
  /* URL of the fetch currently in flight. Copied because the callbacks run
   * long after load_from_url() returned and its argument may be a temporary. */
  char fetch_url[512];

  double previous_time;
  double frame_time;
  RenderTexture2D tgt;
  Rectangle tgt_rect;
  bool sim_loaded;

  bool layer_vis[MAX_LAYERS];
  Btn btn_layer_v[MAX_LAYERS];
  Btn btn_pause;
  Btn btn_rewind;
  Btn btn_sound;
  Btn btn_forward;
  Btn btn_fullrewind;
  Btn btn_clockopt[6];

  Btn btn_copy_url;
  int clock_speed;

  //  struct {
  //    const char* name;
  //    const char* local_path;
  //    const char* url_path;
  //  }* examples;

} C = {0};

static void toggle_simu_pause() { C.paused = !C.paused; }
static void toggle_sound() { C.sound_on = !C.sound_on; }

static struct {
  const char* name;
  const char* local_path;
  const char* url_path;
} examples[] = {
    {"32bit Barrel Shifter", "/assets/example2.png", NULL},
    {"Tutorial", "/assets/circuits/help.png", NULL},
    {"Remote", NULL, "https://i.imgur.com/rolTNUw.png"},
    {"r2", NULL,
     "https://raw.githubusercontent.com/lets-all-be-stupid-forever/"
     "circuit-artist/refs/heads/main/campaign_solutions/02_sevseg1/"
     "sevenseg.png"},
    {0},
};

#if defined(PLATFORM_WEB)
EMSCRIPTEN_KEEPALIVE int ca_get_tick(void) { return C.sim.state.cur_tick; }

EMSCRIPTEN_KEEPALIVE int ca_toggle_pause(void) {
  toggle_simu_pause();
  return C.paused ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int ca_is_paused(void) { return C.paused ? 1 : 0; }
#endif

static float get_speed_dt(int speed) {
  double v1 = C.sim.base_tps;
  switch (speed) {
    case 0:
      return v1 / 16;
      break;
    case 1:
      return v1 / 4;
      break;
    case 2:
      return v1;
      break;
    case 3:
      return v1 * 4;
      break;
    case 4:
      return v1 * 32;
      break;
    case 5:
      return v1 * 128 * 4;
      break;
  }
  return 10000;
}

static void update_layout() {
  Layout* l = C.layout;
  Rectangle canvas = layout_rect(l, "canvas");
  Rectangle window = layout_rect(l, "window");

  int right_margin = (int)(window.width - (canvas.x + canvas.width));
  int bottom_margin = (int)(window.height - (canvas.y + canvas.height));
  // C.target_pos = (Vector2){.x = canvas.x, .y = canvas.y};
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int svg_h = (int)layout_rect(l, "window").height;
  int svg_w = (int)layout_rect(l, "window").width;
  int dy = GetScreenHeight() - svg_h;
  int dx = GetScreenWidth() - svg_w;
  Rectangle lr = layout_rect(l, "example_list");

  canvas.width += dx;
  canvas.height += dy;

  C.tgt_rect = canvas;
  C.viewport = (RecI){
      C.tgt_rect.x,
      C.tgt_rect.y,
      C.tgt_rect.width,
      C.tgt_rect.height,
  };

  C.btn_pause.hitbox = layout_rectb(l, "btn_pause");
  C.btn_sound.hitbox = layout_rectb(l, "btn_sound");
  C.btn_rewind.hitbox = layout_rectb(l, "btn_rewind");
  C.btn_forward.hitbox = layout_rectb(l, "btn_forward");
  C.btn_fullrewind.hitbox = layout_rectb(l, "btn_none");

  lr.x += dx;
  lr.height += dy;
  listbox_set_box(&C.lb, lr);

  C.btn_copy_url.hitbox = layout_rectb(l, "btn_copy_url");
  Rectangle extra[8];
  for (int i = 0; i < 8; i++) {
    extra[i] = layout_rectb(l, TextFormat("btn_extra%d", i + 1));
  }
  for (int i = 0; i < 4; i++) C.btn_clockopt[i].hitbox = extra[i];

  // l->off.y += dy;
  for (int i = 0; i < MAX_LAYERS; i++) {
    // C.btn_layer[i].hitbox = layout_rectb(l, TextFormat("btn_layer%d", i +
    // 1));
    C.btn_layer_v[i].hitbox =
        layout_rectb(l, TextFormat("btn_layer%d_hide", i + 1));
  }
  // l->off.y -= dy;
}

static void player_center_camera() {
  C.cam_idx = cam2d_find_zoom_to_fit(C.img_size, C.viewport);
  C.cam.sp = CAM2D_ZOOM_LUT[C.cam_idx];
  cam2d_center(&C.cam, C.viewport, C.img_size);
}

int compile_and_load(Image img) {
  if (img.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  }

  Image tmp[20];
  image_decode_layers(img, &C.nl, tmp);
  if (C.nl == 1) {
    tmp[0] = ensure_size_multiple_of(tmp[0], 8);
  }
  for (int i = 0; i < C.nl; i++) {
    C.buffer[i] = tmp[i];
    C.t_buffer[i] = clone_texture_from_image(C.buffer[i]);
    C.layer_vis[i] = true;
  }
  C.img_size = (v2i){C.buffer[0].width, C.buffer[0].height};
  C.cam_idx = 7;
  C.cam.sp = CAM2D_ZOOM_LUT[C.cam_idx];
  player_center_camera();
  sim_init(&C.sim, (SimParams){.warmup_cycles = 1,
                               .nl = C.nl,
                               .img = C.buffer,
                               .layers = C.t_buffer,
                               .api = &C.api});
  sim_compile(&C.sim);
  Status s = sim_post_compile(&C.sim);
  C.sim_loaded = true;
  C.paused = true;
  C.hsim = wrap_sim(&C.sim);
  C.simu_target_steps = 0;
  C.pix_toggle = -1;
  C.hover_pix = -1;
  C.time_open = false;
  if (sim_has_errors(&C.sim)) {
    // play_sound_oops();
    C.mode = MODE_SIM_ERROR;
    return -1;
  }
  C.mode = MODE_SIMU;

  if (!s.ok) {
    fprintf(stderr, "Error in sim \n");
    sim_destroy(&C.sim);
  }
  return 0;
}
static void unload_sim() {
  if (C.sim_loaded) {
    hsim_destroy(&C.hsim);
    sim_destroy(&C.sim);
    C.sim_loaded = false;
  }
}

static void unload_image() {
  unload_sim();
  for (int i = 0; i < MAX_LAYERS; i++) {
    if (C.buffer[i].width > 0) {
      UnloadImage(C.buffer[i]);
      UnloadRenderTexture(C.t_buffer[i]);
    }
    C.buffer[i] = (Image){0};
    C.t_buffer[i] = (RenderTexture2D){0};
  }
  C.mode = MODE_EMPTY;
}

static void load_from_local_image(const char* path) {
  unload_image();
  Image img = LoadImage(path);
  if (img.data == NULL) {
    fprintf(stderr, "could not load image: %s\n", path);
    C.mode = MODE_LOAD_IMG_ERROR;
    return;
  }
  compile_and_load(img);
}

static void load_from_png_bytes(const unsigned char* data, int len) {
  unload_image();
  Image img = LoadImageFromMemory(".png", data, len);
  if (img.data == NULL) {
    fprintf(stderr, "could not decode png (%d bytes)\n", len);
    C.mode = MODE_LOAD_IMG_ERROR;
    return;
  }
  compile_and_load(img);
}

/* Perform a load requested from outside the frame loop. */
static void consume_pending_load(void) {
  if (!C.pending_load) return;
  C.pending_load = false;
  if (C.pending_png != NULL) {
    load_from_png_bytes(C.pending_png, C.pending_png_len);
    free(C.pending_png);
    C.pending_png = NULL;
    C.pending_png_len = 0;
  } else {
    load_from_local_image(C.pending_path);
  }
}

#if defined(PLATFORM_WEB)

EMSCRIPTEN_KEEPALIVE void ca_load_image(const char* path) {
  free(C.pending_png); /* a queued-but-unconsumed PNG loses to this call */
  C.pending_png = NULL;
  C.pending_png_len = 0;
  snprintf(C.pending_path, sizeof(C.pending_path), "%s", path);
  C.pending_load = true;
}

EMSCRIPTEN_KEEPALIVE void ca_load_png(const unsigned char* data, int len) {
  free(C.pending_png);
  C.pending_png = malloc(len);
  if (C.pending_png == NULL) {
    C.pending_png_len = 0;
    C.mode = MODE_LOAD_IMG_ERROR;
    return;
  }
  memcpy(C.pending_png, data, len);
  C.pending_png_len = len;
  C.pending_path[0] = '\0';
  C.pending_load = true;
}

#endif

static float get_simu_slack_steps() {
  return C.simu_target_steps - C.sim.state.cur_tick;
}

static float get_simu_dt() { return get_speed_dt(C.clock_speed); }

static int get_simu_speed() {
  if (C.rewind_pressed) return -1;
  if (C.forward_pressed) return 1;
  return (C.paused || C.time_open) ? 0 : 1;
}

static void init_examples() {
  listbox_clear(&C.lb);
  int i = 0;
  while (examples[i].name) {
    listbox_add_row(&C.lb, examples[i].name);
    i++;
  }
}

void player_init() {
  init_common_globals();
  init_i18n();
  widgets_init();
  uifont_load();
  shaders_init();
  InitAudioDevice();
  sound_init();
  C.layout = easy_load_layout("main_player");
  layout_scale(C.layout);
  listbox_init(&C.lb);
  C.lb.row_pad = 1;
  init_examples();
  update_layout();
  C.clock_speed = 2;
  C.sound_on = true;
}

static void simu_play_sounds() {
  int na = arrlen(C.sim.ui_events);
  for (int i = 0; i < na; i++) {
    SimUiEvent ev = C.sim.ui_events[i];
    if (ev.sound > 0 && C.sound_on) {
      play_sound_nand();
    }
  }
}

static Status player_update_simu() {
  Status status = status_ok();
  Sim* pSim = &C.sim;
  float slack_steps = get_simu_slack_steps();

  /* interaction doesn't work when going backward */
  if (C.pix_toggle > -1 && slack_steps >= 0.f) {
    sim_toggle_pixel(pSim, C.pix_toggle);
    hsim_clear_forward_history(&C.hsim);
    C.pix_toggle = -1;
    C.simu_target_steps = C.sim.state.cur_tick + 1;
  }
  slack_steps = get_simu_slack_steps();

  while (status.ok && slack_steps < 0.f) {
    /* Can't go backward */
    if (!hsim_has_prv(&C.hsim)) {
      slack_steps = 0.f;
      C.simu_target_steps = C.sim.state.cur_tick;
      break;
    }
    status = hsim_prv(&C.hsim);
    slack_steps += 1.f;
  }

  while (status.ok && slack_steps >= 1.f) {
    status = hsim_nxt(&C.hsim);
    if (!status.ok) {
      break;
    }
    simu_play_sounds();
    /* Simulation has called Pause() */
    if (C.sim.pause_requested) {
      C.sim.pause_requested = false;
      C.paused = true;
      C.simu_target_steps = C.sim.state.cur_tick;
      // play_sound_click();
      break;
    }
    slack_steps -= 1.f;
  }
  return status;
}

/* Same as win_main: signed angle (degrees) swept from a to b around c. */
static float get_clock_delta(v2 c, v2 a, v2 b) {
  float ay = a.y - c.y;
  float ax = a.x - c.x;
  float by = b.y - c.y;
  float bx = b.x - c.x;
  float ra = sqrt(ax * ax + ay * ay + 1e-1);
  float rb = sqrt(bx * bx + by * by + 1e-1);
  float cross = -ax * by + ay * bx;
  return (180 / M_PI) * asin(cross / (ra * rb));
}

static void ensure_camera_bounds() {
  cam2d_ensure_bounds(&C.cam, C.img_size, C.viewport, (v2i){0, 0});
}

/* Keeps the viewport in sync with the target size; recenters on first use. */
// static void player_set_viewport(int w, int h) {
//   bool first = (C.viewport.width == 0);
//   C.viewport = (RecI){0, 0, w, h};
//   if (first) {
//     player_center_camera();
//   } else {
//     ensure_camera_bounds();
//   }
// }

static bool is_mouse_on_canvas() {
  Vector2 mouse = GetMousePosition();
  return CheckCollisionPointRec(mouse, C.tgt_rect);
}

static void update_camera(double frame_time) {
  v2 mouse = GetMousePosition();

  /* Wheel zoom around the cursor */
  float wheel = GetMouseWheelMove();
  if (is_mouse_on_canvas()) {
    if (fabs(wheel) > 1e-3) {
      cam2d_zoom_at(&C.cam, &C.cam_idx, C.viewport, mouse, wheel > 0 ? 1 : -1);
      ensure_camera_bounds();
      // play_sound_click();
    }
  }

  /* -/= zoom around the viewport center */
  int zoom = IsKeyPressed(KEY_EQUAL) - IsKeyPressed(KEY_MINUS);
  if (zoom != 0) {
    v2 center = {C.viewport.x + C.viewport.width / 2.0,
                 C.viewport.y + C.viewport.height / 2.0};
    cam2d_zoom_at(&C.cam, &C.cam_idx, C.viewport, center, zoom);
    ensure_camera_bounds();
    // play_sound_click();
  }

  /* Middle button drag pan */
  if (is_mouse_on_canvas() && IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
    v2 d = GetMouseDelta();
    C.cam.off.x += d.x;
    C.cam.off.y += d.y;
    ensure_camera_bounds();
  }

  /* WASD pan */
  if (!is_control_down()) {
    int dy = IsKeyDown(KEY_W) - IsKeyDown(KEY_S) + IsKeyDown(KEY_UP) -
             IsKeyDown(KEY_DOWN);
    int dx = IsKeyDown(KEY_A) - IsKeyDown(KEY_D) + IsKeyDown(KEY_LEFT) -
             IsKeyDown(KEY_RIGHT);
    C.pan_acc_x = dx ? C.pan_acc_x + dx * PAN_SPEED * frame_time : 0;
    C.pan_acc_y = dy ? C.pan_acc_y + dy * PAN_SPEED * frame_time : 0;
    int sx = (int)round(C.pan_acc_x);
    int sy = (int)round(C.pan_acc_y);
    C.pan_acc_x -= sx;
    C.pan_acc_y -= sy;
    if (sx || sy) {
      C.cam.off.x += sx;
      C.cam.off.y += sy;
      ensure_camera_bounds();
    }
  }
}

static void update_wire_toggle() {
  C.hover_pix = -1;
  if (C.time_open) return;
  if (!is_mouse_on_canvas()) return;
  v2 mouse = GetMousePosition();
  v2 fpix = cam2d_screen_to_image(C.cam, C.viewport, mouse);
  int px = (int)floorf(fpix.x);
  int py = (int)floorf(fpix.y);
  if (px < 0 || px >= C.img_size.x || py < 0 || py >= C.img_size.y) return;

  int pix = -1;
  sim_find_nearest_pixel(&C.sim, TOGGLE_SEARCH_RADIUS, fpix, &pix);
  if (pix == -1) return;
  C.hover_pix = pix;
  if (!sim_has_errors(&C.sim) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    C.pix_toggle = pix;
    C.paused = false;
  }
}

static void full_rewind() {
  Status s = sim_reset_state(&C.sim);
  if (!s.ok) {
    // TODO: need to handle kernel error ...
    // handle_kernel_error(s);
    return;
  }
  hsim_destroy(&C.hsim);
  C.hsim = wrap_sim(&C.sim);
  C.simu_target_steps = 0;
  C.pix_toggle = -1;
}

static void update_time_controls() {
  C.forward_pressed = false;
  C.rewind_pressed = false;

  if (C.mode == MODE_SIMU) {
    if (IsKeyPressed(KEY_K) || IsKeyPressed(KEY_SPACE)) C.paused = !C.paused;
    if (IsKeyDown(KEY_L)) C.forward_pressed = true;
    if (IsKeyDown(KEY_J)) C.rewind_pressed = true;
    if (IsKeyDown(KEY_R)) full_rewind();
  }
  if (IsKeyPressed(KEY_M)) toggle_sound();

  /* Right button: time-travel clock. Dragging around the viewport center
   * scrubs the simulation forward/backward. */
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && is_mouse_on_canvas()) {
    C.time_open = true;
    C.time_ref = C.simu_target_steps;
    C.time_pos_ref = GetMousePosition();
    C.time_c = (v2){C.viewport.x + C.viewport.width / 2.0,
                    C.viewport.y + C.viewport.height / 2.0};
  }
  if (C.time_open && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    C.time_open = false;
  }
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
    C.time_open = false;
  }
}

static void draw_status_bar() {
  Color tc = CA_WHITE;
  Color bg = BLACK;
  int lh = uifont_line_height();
  int step = lh + 2;
  int num_lines = 7;
  int xc = 8 + C.tgt_rect.x;
  int ybot = C.tgt_rect.y + C.tgt_rect.height;
  int yc1 = ybot - step * num_lines;
  int yc2 = yc1 + step;
  int yc3 = yc2 + step;
  int yc4 = yc3 + step;
  int yc5 = yc4 + step;
  int yc6 = yc5 + step;
  int yc7 = yc6 + step;

  char txt[500];

  if (is_mouse_on_canvas()) {
    // v2i cursor = paint_get_cursor(pnt);
    v2 mouse = GetMousePosition();
    v2 fpix = cam2d_screen_to_image(C.cam, C.viewport, mouse);
    v2i cursor = {fpix.x, fpix.y};
    snprintf(txt, sizeof(txt), T.main_bar_coord, cursor.x, cursor.y);
    uifont_draw_texture_outlined(txt, xc, yc1, tc, bg);
  }
  int zoomPerc = 100 * CAM2D_ZOOM_LUT[C.cam_idx];
  snprintf(txt, sizeof(txt), "Zoom: %d%%", zoomPerc);
  uifont_draw_texture_outlined(txt, xc, yc2, tc, bg);

  int yc = yc3;
  if (C.mode == MODE_SIM_ERROR) {
    uifont_draw_texture_outlined("Circuit has errors!", xc, yc, tc, bg);
    yc += step;
  } else {
    if (C.mode == MODE_SIMU) {
      int num_nands = arrlen(C.sim.pg.nands);
      snprintf(txt, sizeof(txt), "NANDs: %s", fmtnum(num_nands));
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;
      bool warmup = sim_is_on_warmup(&C.sim);
      if (warmup) {
        // E, C, T, CRIT:
        // uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
        // yc += step;
        uifont_draw_texture_outlined("Cycle: (warmup)", xc, yc, tc, bg);
        yc += step;
        uifont_draw_texture_outlined("Tick: (warmup)", xc, yc, tc, bg);
        yc += step;
        uifont_draw_texture_outlined("CRIT: (warmup)", xc, yc, tc, bg);
        yc += step;
      } else {
        // snprintf(txt, sizeof(txt), T.main_bar_energy,
        //          fmtnum((int)(C.sim.state.total_energy)));
        // uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
        // yc += step;
        int cycle = sim_get_effective_cycle(&C.sim);

        snprintf(txt, sizeof(txt), "Cycle: %d", cycle);
        uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
        yc += step;

        int t = C.sim.state.cur_period_tick + 1;
        snprintf(txt, sizeof(txt), "Tick: %d", t);
        uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
        yc += step;
        snprintf(txt, sizeof(txt), "CRIT: C: %d T: %d",
                 C.sim.state.max_tick_cycle, C.sim.state.max_tick);
        uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
        yc += step;
      }
    }
  }
  v2i buf_size = C.img_size;
  snprintf(txt, sizeof(txt), "Size: w: %d h: %d", buf_size.x, buf_size.y);
  uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
}

static void draw_ui() {
  btn_draw_icon(&C.btn_pause, C.paused ? rect_start : rect_pause);
  btn_draw_icon(&C.btn_sound, C.sound_on ? rect_sound_on : rect_sound_off);
  btn_draw_icon(&C.btn_fullrewind, rect_full_rewind);
  btn_draw_icon(&C.btn_rewind, rect_rewind);
  btn_draw_icon(&C.btn_forward, rect_forward);
  btn_draw_icon(&C.btn_clockopt[0], rect_hz0);
  btn_draw_icon(&C.btn_clockopt[1], rect_hz1);
  btn_draw_icon(&C.btn_clockopt[2], rect_hz4);
  btn_draw_icon(&C.btn_clockopt[3], rect_hz16);
  int rh = C.lb.row_hit;
  listbox_draw(&C.lb, rh);
  btn_draw_text(&C.btn_copy_url, "Copy Link");

  // btn_draw_icon(&C.btn_clockopt[4], rect_hz64);
  // btn_draw_icon(&C.btn_clockopt[5], rect_hz1k);
  for (int i = 0; i < C.nl; i++) {
    bool v = C.layer_vis[i];
    btn_draw_icon(&C.btn_layer_v[i], v ? rect_eye_on : rect_eye_off);
  }
  if (C.mode == MODE_SIM_ERROR || C.mode == MODE_SIMU) {
    draw_status_bar();
  }
}

#if defined(PLATFORM_WEB)
/* emscripten hands back a buffer it owns and frees as soon as this returns,
 * and the callback runs between frames -- both of which ca_load_png() already
 * handles: it copies the bytes into the pending slot for the next frame to
 * decode, so no sim teardown or GL work happens out here. */
static void on_url_loaded(void* arg, void* data, int size) {
  (void)arg;
  ca_load_png((const unsigned char*)data, size);
}

static void on_url_error(void* arg) {
  (void)arg;
  /* Usually CORS rather than a dead link: a cross-origin fetch needs
   * access-control-allow-origin from the host, which many image hosts omit.
   * The browser deliberately gives no detail, so there is nothing to report
   * beyond the URL. */
  fprintf(stderr, "could not fetch %s\n", C.fetch_url);
  C.mode = MODE_LOAD_IMG_ERROR;
}
#endif

/* Fetch an image over HTTP and load it.
 *
 * No JS and no shell involvement: emscripten_async_wget_data() is emscripten's
 * own C binding over XHR, needs no -sFETCH, and is callback-based so it does
 * not interact with ASYNCIFY (a synchronous fetch would unwind the frame
 * mid-render). */
static void load_from_url(const char* url) {
#if defined(PLATFORM_WEB)
  /* Two clicks in quick succession leave two fetches in flight; the last to
   * arrive wins, the same rule every other load path follows. */
  snprintf(C.fetch_url, sizeof(C.fetch_url), "%s", url);
  emscripten_async_wget_data(C.fetch_url, NULL, on_url_loaded, on_url_error);
#else
  fprintf(stderr, "loading from a URL needs the web build: %s\n", url);
#endif
}

static void load_example(int i) {
  unload_image();
  if (examples[i].local_path) {
    load_from_local_image(examples[i].local_path);
  } else {
    C.mode = MODE_LOAD_IMG_PENDING;
    load_from_url(examples[i].url_path);
  }
}

static void update_ui_controls() {
  for (int i = 0; i < C.nl; i++) {
    if (btn_update(&C.btn_layer_v[i])) {
      C.layer_vis[i] = !C.layer_vis[i];
    }
  }
  if (btn_update(&C.btn_clockopt[0])) C.clock_speed = 0;
  if (btn_update(&C.btn_clockopt[1])) C.clock_speed = 1;
  if (btn_update(&C.btn_clockopt[2])) C.clock_speed = 2;
  if (btn_update(&C.btn_clockopt[3])) C.clock_speed = 3;
  // if (btn_update(&C.btn_clockopt[4])) C.clock_speed = 4;
  // if (btn_update(&C.btn_clockopt[5])) C.clock_speed = 5;
  if (btn_update(&C.btn_pause)) toggle_simu_pause();
  if (btn_update(&C.btn_sound)) toggle_sound();
  btn_update(&C.btn_rewind);
  btn_update(&C.btn_forward);
  if (C.btn_rewind.pressed) C.rewind_pressed = true;
  if (C.btn_forward.pressed) C.forward_pressed = true;
  if (btn_update(&C.btn_fullrewind)) full_rewind();
  listbox_update(&C.lb);
  int hit = C.lb.row_hit;
  if (hit >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    // TODO: load example!
    load_example(hit);
    // set_sel(C.topic_sel, hit);
  }

  bool sm = C.mode == MODE_SIMU;
  for (int i = 0; i < C.nl; i++) {
    C.btn_layer_v[i].toggled = sm && C.layer_vis[i];
  }
  C.btn_rewind.toggled = sm && C.rewind_pressed;
  C.btn_forward.toggled = sm && C.forward_pressed;
  C.btn_forward.disabled = sm && !C.paused;
  for (int i = 0; i < 4; i++) {
    C.btn_clockopt[i].disabled = !sm;
    C.btn_clockopt[i].toggled = C.clock_speed == i;
  }
}

void player_update(double frame_time) {
  if (C.viewport.width == 0) return; /* No target yet */
  update_camera(frame_time);
  update_time_controls();
  update_ui_controls();

  bool mouse_on_canvas = is_mouse_on_canvas();

  if (C.mode == MODE_SIMU) update_wire_toggle();

  SetMouseCursor(C.hover_pix != -1 ? MOUSE_CURSOR_POINTING_HAND
                                   : MOUSE_CURSOR_DEFAULT);

  if (C.mode == MODE_SIMU) {
    double dt = get_simu_dt();
    if (C.time_open) {
      v2 pos = GetMousePosition();
      v2 ref = C.time_pos_ref;
      double diff =
          CLOCK_SENSITIVITY * dt * get_clock_delta(C.time_c, pos, ref);
      double tgt = C.time_ref + diff;
      if (tgt < 0) {
        C.time_ref += tgt;
        tgt = 0;
      }
      C.simu_target_steps = tgt;
      C.time_ref = tgt;
      C.time_pos_ref = pos;
    } else {
      int dir = get_simu_speed();
      if (dir != 0) {
        double new_time = C.simu_target_steps + frame_time * dir * dt;
        C.simu_target_steps = new_time >= 0 ? new_time : 0;
      }
    }
    Status s = player_update_simu();
    if (!s.ok) {
      fprintf(stderr, "sim error: %s\n", s.err_msg ? s.err_msg : "");
      C.mode = MODE_SIM_ERROR;
    }
  }
}

static void draw_hud() {
  const char* state = C.time_open ? "TIME TRAVEL"
                      : C.paused  ? "PAUSED"
                                  : "RUNNING";
  DrawText(TextFormat("T=%d  %s", C.sim.state.cur_tick, state), 10, 10, 20,
           WHITE);
  DrawText(
      "click: toggle wire | wheel/-/=: zoom | MMB/WASD: pan | "
      "K: pause | J/L: rewind/forward | RMB drag: time travel",
      10, GetScreenHeight() - 26, 16, GRAY);
}

void player_render(RenderTexture2D* target) {
  int tw = target->texture.width;
  int th = target->texture.height;
  // player_set_viewport(tw, th);

  float slack_steps = get_simu_slack_steps();
  float frame_steps = get_simu_dt() * 1.0 / 60.0;
  if (C.paused) frame_steps = 0;
  bool neon = true;
  Color bg = {41, 31, 13, 255};
  int hide_mask = 0;
  for (int i = 0; i < C.nl; i++) {
    if (!C.layer_vis[i]) {
      hide_mask = hide_mask | (1 << i);
    }
  }
  Tex* rendered = sim_render_v2(&C.sim, tw, th, C.cam, frame_steps, slack_steps,
                                hide_mask, neon, bg);
  texdraw2(*target, rendered->rt);
  texdel(rendered);

  if (C.time_open) {
    /* texclock renders into its own target, so do it before drawing on ours
     */
    Tex* clock = texnew(tw, th);
    texclear(clock, BLANK);
    v2 pos = GetMousePosition();
    texclock(clock, pos.x - C.time_c.x, pos.y - C.time_c.y);
    BeginTextureMode(*target);
    DrawTexturePro(clock->rt.texture, (Rectangle){0, 0, tw, -th},
                   (Rectangle){0, 0, tw, th}, (Vector2){0, 0}, 0, WHITE);
    EndTextureMode();
    texdel(clock);
  }

  BeginTextureMode(*target);
  EndTextureMode();

  /* Release pool textures that went unused this frame (e.g. buffers of a
   * previous window size after a resize); the pool aborts past 50 entries. */
  texcleanup();
}

static void frame_control_spin() {
  double target_fps = 60;
  SwapScreenBuffer();
  double current_time = GetTime();
  double update_draw_time = current_time - C.previous_time;
  double wait_time = (1.0 / target_fps) - update_draw_time;
  if (wait_time < 0) wait_time = 0;
  double eps = 0.002;
  double target = wait_time + current_time;
  if (target - current_time > eps) {
    WaitTime(target - current_time - eps);
    current_time = GetTime();
  }
  // busy - wait
  while (current_time < target) {
    current_time = GetTime();
  }
  double delta_time = current_time - C.previous_time;
  C.frame_time = delta_time;
  C.previous_time = current_time;
  PollInputEvents();
}

#if defined(PLATFORM_WEB)
/* On web the browser paces us via requestAnimationFrame, so there is nothing
 * to wait/spin on: just swap, measure the elapsed time and poll input.
 * raylib is built with SUPPORT_CUSTOM_FRAME_CONTROL, so EndDrawing() does
 * none of this for us. */
static void frame_control_web() {
  SwapScreenBuffer();
  double current_time = GetTime();
  double delta_time = current_time - C.previous_time;
  /* rAF stops while the tab is hidden; don't let the sim jump on return. */
  if (delta_time > 0.1) delta_time = 0.1;
  C.frame_time = delta_time;
  C.previous_time = current_time;
  PollInputEvents();
}
#endif

static void _draw_rt_on_screen(RenderTexture2D rt, Vector2 pos) {
  int tw = rt.texture.width;
  int th = rt.texture.height;
  Rectangle source = {
      .x = 0,
      .y = 0,
      .width = tw,
      .height = -th,
  };
  Rectangle target = {
      .x = pos.x,
      .y = pos.y,
      .width = tw,
      .height = th,
  };
  Vector2 v0 = {0};
  DrawTexturePro(rt.texture, source, target, v0, 0.0f, WHITE);
}

static void check_size(RenderTexture2D* t) {
  int sw = C.tgt_rect.width;
  int sh = C.tgt_rect.height;
  // int sw = GetRenderWidth();
  // int sh = GetRenderHeight();
  int tw = t->texture.width;
  int th = t->texture.height;
  if (tw != sw || th != sh) {
    UnloadRenderTexture(*t);
    *t = LoadRenderTexture(sw, sh);
  }
}

/* An image dragged onto the window.
 *
 * No JS needed: emscripten's GLFW already listens for drop/dragover on the
 * canvas, writes each dropped file into MEMFS (/.glfw_dropped_files/<name>)
 * and only then invokes the drop callback, which raylib's web backend wires
 * into the same IsFileDropped() API it uses on desktop. So the path handed to
 * us is a readable file on both platforms and this needs no #ifdef.
 *
 * Goes through the pending-load slot instead of loading inline, so the sim
 * teardown and GL texture rebuild happen at the same point in the frame as
 * every other load (see ca_load_image). */
static void check_dropped_files(void) {
  if (!IsFileDropped()) return;
  FilePathList dropped = LoadDroppedFiles();
  if (dropped.count > 0) {
    /* Last one wins when several are dropped together. */
    const char* path = dropped.paths[dropped.count - 1];
    /* Only the formats this raylib is actually built with; anything else
     * would come back as a zeroed Image and drop us into MODE_LOAD_ERROR,
     * throwing away the circuit already on screen over a stray drop. */
    if (IsFileExtension(path, ".png;.bmp;.gif;.qoi")) {
      free(C.pending_png); /* a queued-but-unconsumed PNG loses to this */
      C.pending_png = NULL;
      C.pending_png_len = 0;
      snprintf(C.pending_path, sizeof(C.pending_path), "%s", path);
      C.pending_load = true;
    } else {
      fprintf(stderr, "ignoring dropped file (unsupported type): %s\n", path);
    }
  }
  /* Mandatory: LoadDroppedFiles() returns raylib's internal arrays without
   * clearing the count, so skipping this leaks and leaves IsFileDropped()
   * true forever. It also frees the paths, hence the copy above. */
  UnloadDroppedFiles(dropped);
}

static void render_load_error(RenderTexture2D* target) {
  BeginTextureMode(*target);
  ClearBackground(RED);
  Color tc = CA_WHITE;
  Color bg = BLACK;
  uifont_draw_texture_outlined("Error: File is not a valid PNG image.", 10, 10,
                               tc, bg);
  EndTextureMode();
}

static void draw_ui_legend() {
  for (int i = 0; i < C.nl; i++) {
    btn_draw_legend(&C.btn_layer_v[i],
                    TextFormat(T.main_layer_show_leg, i + 1));
  }
  btn_draw_legend(&C.btn_rewind, T.main_rewind_leg);
  btn_draw_legend(&C.btn_sound, "Mute/Unmute sound (M)");
  btn_draw_legend(&C.btn_forward, T.main_forward_leg);
  btn_draw_legend(&C.btn_fullrewind, T.main_fullrewind_leg);
  btn_draw_legend(&C.btn_pause, "Play/Pause Simulation (K / Space)\n");
  btn_draw_legend(
      &C.btn_clockopt[0],
      TextFormat(T.main_speed1_leg, fmtnum((int)(get_speed_dt(0)))));
  btn_draw_legend(
      &C.btn_clockopt[1],
      TextFormat(T.main_speed2_leg, fmtnum((int)(get_speed_dt(1)))));
  btn_draw_legend(
      &C.btn_clockopt[2],
      TextFormat(T.main_speed3_leg, fmtnum((int)(get_speed_dt(2)))));
  btn_draw_legend(
      &C.btn_clockopt[3],
      TextFormat(T.main_speed4_leg, fmtnum((int)(get_speed_dt(3)))));
  //   btn_draw_legend(
  //       &C.btn_clockopt[4],
  //       TextFormat(T.main_speed5_leg, fmtnum((int)(get_speed_dt(4)))));
  //   btn_draw_legend(
  //       &C.btn_clockopt[5],
  //       TextFormat(T.main_speed6_leg, fmtnum((int)(get_speed_dt(5)))));
}

static void render_empty(RenderTexture2D* target) {
  BeginTextureMode(*target);
  ClearBackground(BLACK);
  Color tc = CA_WHITE;
  Color bg = BLACK;
  uifont_draw_texture_outlined("Empty", 10, 10, tc, bg);
  EndTextureMode();
}

static void render_loading(RenderTexture2D* target) {
  BeginTextureMode(*target);
  ClearBackground(BLACK);
  Color tc = CA_WHITE;
  Color bg = BLACK;
  uifont_draw_texture_outlined("Loading...", 10, 10, tc, bg);
  EndTextureMode();
}

static void update_draw_frame(void) {
  SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  widgets_set_sound(C.sound_on);
  ui_reset_hit_count();
  update_layout();
  check_dropped_files();
  consume_pending_load();
  check_size(&C.tgt);
  player_update(C.frame_time);
  if (C.lb.row_hit != -1) {
    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
  }
  if (C.mode == MODE_EMPTY) {
    render_empty(&C.tgt);
  }
  if (C.mode == MODE_LOAD_IMG_PENDING) {
    render_loading(&C.tgt);
  }
  if (C.mode == MODE_SIMU || C.mode == MODE_SIM_ERROR) {
    player_render(&C.tgt);
  }
  if (C.mode == MODE_LOAD_IMG_ERROR) {
    render_load_error(&C.tgt);
  }
  BeginDrawing();
  ClearBackground(BLACK); /* same as the game behind its sim target */
  draw_default_tiled_screen();
  _draw_rt_on_screen(C.tgt, (Vector2){C.tgt_rect.x, C.tgt_rect.y});
  draw_default_tiled_frame(C.tgt_rect);
  draw_ui();
  draw_default_tiled_frame(C.lb.hitbox);
  draw_ui_legend();
  EndDrawing();
#if defined(PLATFORM_WEB)
  frame_control_web();
#else
  frame_control_spin();
#endif
}

int main(int argc, char** argv) {
#if defined(PLATFORM_WEB)
  /* No HIGHDPI on web: raylib's web backend doesn't scale the canvas by
   * devicePixelRatio, but with the flag set its resize callback still divides
   * the reported screen size by it, so GetScreenWidth() would disagree with
   * the actual canvas size and check_size() would build an undersized target.
   */
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
#else
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
#endif
  InitWindow(1000, 800, "player");
  player_init();
  C.mode = MODE_EMPTY;
  C.tgt = LoadRenderTexture(800, 800);
  C.previous_time = GetTime();

  init_game_registry();
  LevelDef* ldef = get_level_by_id("official:basics");
  lua_level_create(&C.api, ldef);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(update_draw_frame, 0, 1);
#else
  while (!WindowShouldClose()) {
    update_draw_frame();
  }
  printf("hi\n");
#endif

  return 0;
}
