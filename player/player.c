#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cam2d.h"
#include "img.h"
#include "raylib.h"
#include "shaders.h"
#include "sim.h"
#include "sound.h"
#include "stdio.h"
#include "tex.h"
#include "utils.h"

typedef enum {
  MODE_EMPTY,
  MODE_LOADING,
  MODE_LOAD_ERROR,
  MODE_READY,
  MODE_SIM_ERROR,
  MODE_COMPILING,
  MODE_SIMU,
} PlayerMode;

/* Search radius (image pixels) when clicking near a wire to toggle it. */
#define TOGGLE_SEARCH_RADIUS 5
/* Keyboard pan speed, screen pixels per second (same as the paint canvas). */
#define PAN_SPEED 600.0
/* Time-travel clock sensitivity: ticks per degree of rotation, scaled by tps.
 */
#define CLOCK_SENSITIVITY 0.003

// States:
//   Empty
//   Ready
//   ErrorLoad
//   Running
//   Error
//
static struct {
  LevelAPI api;
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

  /* Load requested from outside the frame loop (JS), consumed at the top of
   * the next frame. See ca_load_image() / ca_load_png(). Exactly one of the
   * two is set: pending_png wins when non-NULL. */
  bool pending_load;
  char pending_path[512];
  unsigned char* pending_png; /* owned here, freed once decoded */
  int pending_png_len;

  double previous_time;
  double frame_time;
  RenderTexture2D tgt;
} C = {0};

#if defined(PLATFORM_WEB)
EMSCRIPTEN_KEEPALIVE int ca_get_tick(void) { return C.sim.state.cur_tick; }

/* Pause/resume from the HTML overlay. This is the same flag the K key toggles
 * in update_time_controls(), so the two stay interchangeable. Returns the new
 * state so the caller can relabel its button without a second call.
 * Safe to call from a DOM handler: those run between frames, never inside
 * update_draw_frame(), so nothing can observe a half-applied change. */
EMSCRIPTEN_KEEPALIVE int ca_toggle_pause(void) {
  C.paused = !C.paused;
  return C.paused ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int ca_is_paused(void) { return C.paused ? 1 : 0; }
#endif

int compile_and_load(Image img) {
  /* Everything downstream reads pixels as Color*, and get_pixels() asserts on
   * it, but raylib keeps an alpha-less PNG as R8G8B8 -- 3 bytes per pixel, not
   * 4. Every bundled asset happens to be RGBA so this never came up; a PNG
   * handed in from JS is whatever the author saved. */
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
  }
  C.img_size = (v2i){C.buffer[0].width, C.buffer[0].height};
  C.cam_idx = 7;
  C.cam.sp = CAM2D_ZOOM_LUT[C.cam_idx];
  C.viewport = (RecI){0};
  sim_init(&C.sim, (SimParams){.warmup_cycles = 0,
                               .nl = C.nl,
                               .img = C.buffer,
                               .layers = C.t_buffer,
                               .api = &C.api});
  sim_compile(&C.sim);
  Status s = sim_post_compile(&C.sim);

  C.hsim = wrap_sim(&C.sim);
  C.simu_target_steps = 0;
  C.pix_toggle = -1;
  C.hover_pix = -1;
  C.paused = false;
  C.time_open = false;
  if (sim_has_errors(&C.sim)) {
    play_sound_oops();
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

static void stop_simulation() {}

static void unload_image() {
  // TODO: Stop sim if need.
  // TODO: Unload all stuff...
  C.mode = MODE_EMPTY;
}

static void load_from_local_image(const char* path) {
  unload_image();
  Image img = LoadImage(path);
  /* raylib reports failure by handing back a zeroed Image; decoding that walks
   * a NULL pixel buffer. Mattered less when the path was a literal, but JS
   * picks it now. */
  if (img.data == NULL) {
    fprintf(stderr, "could not load image: %s\n", path);
    C.mode = MODE_LOAD_ERROR;
    return;
  }
  compile_and_load(img);
}

/* Same thing from an encoded PNG already in memory, so a caller that obtained
 * the bytes some other way (an HTTP fetch, a file drop) doesn't have to invent
 * a file for them. raylib decodes straight from the buffer; ".png" only tells
 * it which decoder to use, it is not a filename. */
static void load_from_png_bytes(const unsigned char* data, int len) {
  unload_image();
  Image img = LoadImageFromMemory(".png", data, len);
  if (img.data == NULL) {
    fprintf(stderr, "could not decode png (%d bytes)\n", len);
    C.mode = MODE_LOAD_ERROR;
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
/* Let JS choose the circuit. `path` is a MEMFS path, so it addresses the
 * assets/ tree baked in by --preload-file (e.g. "/assets/example2.png").
 *
 * This only records the request: the load tears down the sim and creates GL
 * textures, and a DOM handler fires between frames -- outside the rAF callback
 * and outside Begin/EndDrawing. Deferring to the top of the next frame keeps
 * that work where the rest of it already happens, the same way C.pix_toggle
 * defers a wire toggle into player_update_simu(). Poll ca_get_mode_name() for
 * the result. */
EMSCRIPTEN_KEEPALIVE void ca_load_image(const char* path) {
  free(C.pending_png); /* a queued-but-unconsumed PNG loses to this call */
  C.pending_png = NULL;
  C.pending_png_len = 0;
  snprintf(C.pending_path, sizeof(C.pending_path), "%s", path);
  C.pending_load = true;
}

/* Hand over an encoded PNG from JS -- no MEMFS detour needed. `data` points at
 * a buffer JS allocated in the wasm heap; we copy out of it so JS stays free to
 * free it the moment this returns, and the copy survives until the next frame
 * decodes it. */
EMSCRIPTEN_KEEPALIVE void ca_load_png(const unsigned char* data, int len) {
  free(C.pending_png);
  C.pending_png = malloc(len);
  if (C.pending_png == NULL) {
    C.pending_png_len = 0;
    C.mode = MODE_LOAD_ERROR;
    return;
  }
  memcpy(C.pending_png, data, len);
  C.pending_png_len = len;
  C.pending_path[0] = '\0';
  C.pending_load = true;
}

/* Names rather than the raw enum, so JS doesn't carry a copy of PlayerMode
 * that would silently drift when a mode is added. */
EMSCRIPTEN_KEEPALIVE const char* ca_get_mode_name(void) {
  switch (C.mode) {
    case MODE_EMPTY:
      return "empty";
    case MODE_LOADING:
      return "loading";
    case MODE_LOAD_ERROR:
      return "load_error";
    case MODE_READY:
      return "ready";
    case MODE_SIM_ERROR:
      return "sim_error";
    case MODE_COMPILING:
      return "compiling";
    case MODE_SIMU:
      return "simu";
  }
  return "unknown";
}
#endif

static float get_simu_slack_steps() {
  return C.simu_target_steps - C.sim.state.cur_tick;
}

static double get_simu_dt() { return C.sim.base_tps; }

static int get_simu_speed() {
  if (C.rewind_pressed) return -1;
  if (C.forward_pressed) return 1;
  return (C.paused || C.time_open) ? 0 : 1;
}

void player_init() {
  shaders_init();
  InitAudioDevice();
  sound_init();
}

static void simu_play_sounds() {
  int na = arrlen(C.sim.ui_events);
  for (int i = 0; i < na; i++) {
    SimUiEvent ev = C.sim.ui_events[i];
    if (ev.sound > 0) {
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
      play_sound_click();
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

static void player_center_camera() {
  C.cam_idx = cam2d_find_zoom_to_fit(C.img_size, C.viewport);
  C.cam.sp = CAM2D_ZOOM_LUT[C.cam_idx];
  cam2d_center(&C.cam, C.viewport, C.img_size);
}

/* Keeps the viewport in sync with the target size; recenters on first use. */
static void player_set_viewport(int w, int h) {
  bool first = (C.viewport.width == 0);
  C.viewport = (RecI){0, 0, w, h};
  if (first) {
    player_center_camera();
  } else {
    ensure_camera_bounds();
  }
}

static void update_camera(double frame_time) {
  v2 mouse = GetMousePosition();

  /* Wheel zoom around the cursor */
  float wheel = GetMouseWheelMove();
  if (fabs(wheel) > 1e-3) {
    cam2d_zoom_at(&C.cam, &C.cam_idx, C.viewport, mouse, wheel > 0 ? 1 : -1);
    ensure_camera_bounds();
    play_sound_click();
  }

  /* -/= zoom around the viewport center */
  int zoom = IsKeyPressed(KEY_EQUAL) - IsKeyPressed(KEY_MINUS);
  if (zoom != 0) {
    v2 center = {C.viewport.x + C.viewport.width / 2.0,
                 C.viewport.y + C.viewport.height / 2.0};
    cam2d_zoom_at(&C.cam, &C.cam_idx, C.viewport, center, zoom);
    ensure_camera_bounds();
    play_sound_click();
  }

  /* Middle button drag pan */
  if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
    v2 d = GetMouseDelta();
    C.cam.off.x += d.x;
    C.cam.off.y += d.y;
    ensure_camera_bounds();
  }

  /* WASD pan */
  if (!is_control_down()) {
    int dy = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);
    int dx = IsKeyDown(KEY_A) - IsKeyDown(KEY_D);
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

static void update_time_controls() {
  C.forward_pressed = false;
  C.rewind_pressed = false;

  if (IsKeyPressed(KEY_K)) C.paused = !C.paused;
  if (IsKeyDown(KEY_L)) C.forward_pressed = true;
  if (IsKeyDown(KEY_J)) C.rewind_pressed = true;

  /* Right button: time-travel clock. Dragging around the viewport center
   * scrubs the simulation forward/backward. */
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    C.time_open = true;
    C.time_ref = C.simu_target_steps;
    C.time_pos_ref = GetMousePosition();
    C.time_c = (v2){C.viewport.x + C.viewport.width / 2.0,
                    C.viewport.y + C.viewport.height / 2.0};
  }
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
    C.time_open = false;
  }
}

void player_update(double frame_time) {
  if (C.viewport.width == 0) return; /* No target yet */
  update_camera(frame_time);
  update_time_controls();
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
  player_set_viewport(tw, th);

  float slack_steps = get_simu_slack_steps();
  float frame_steps = get_simu_dt() * 1.0 / 60.0;
  if (C.paused) frame_steps = 0;
  int hide_mask = 0;
  bool neon = true;
  Color bg = {41, 31, 13, 255};
  Tex* rendered = sim_render_v2(&C.sim, tw, th, C.cam, frame_steps, slack_steps,
                                hide_mask, neon, bg);
  texdraw2(*target, rendered->rt);
  texdel(rendered);

  if (C.time_open) {
    /* texclock renders into its own target, so do it before drawing on ours */
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
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  // int sw = GetRenderWidth();
  // int sh = GetRenderHeight();
  int tw = t->texture.width;
  int th = t->texture.height;
  if (tw != sw || th != sh) {
    UnloadRenderTexture(*t);
    *t = LoadRenderTexture(sw, sh);
  }
}

static void update_draw_frame(void) {
  consume_pending_load();
  check_size(&C.tgt);
  player_update(C.frame_time);
  if (C.mode == MODE_SIMU || C.mode == MODE_SIM_ERROR) {
    player_render(&C.tgt);
  }
  BeginDrawing();
  ClearBackground(BLACK); /* same as the game behind its sim target */
  _draw_rt_on_screen(C.tgt, (Vector2){0});
  if (C.mode == MODE_EMPTY) {
    DrawText("EMPTY", 40, 40, 20, WHITE);
  }
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
  // load_from_local_image("../assets/example2.png");
  C.tgt = LoadRenderTexture(800, 800);
  C.previous_time = GetTime();

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
