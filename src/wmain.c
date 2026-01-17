#include "wmain.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <msgpack.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "colors.h"
#include "common.h"
#include "font.h"
#include "img.h"
#include "json.h"
#include "level.h"
#include "log.h"
#include "math.h"
#include "modal.h"
#include "msg.h"
#include "paint.h"
#include "profiler.h"
#include "sim.h"
#include "stb_ds.h"
#include "time.h"
#include "tutorial.h"
#include "ui.h"
#include "uigraph.h"
#include "utils.h"
#include "wabout.h"
#include "wdialog.h"
#include "widgets.h"
#include "win_blueprint.h"
#include "win_level.h"
#include "win_stamp.h"
#include "wnumber.h"
#include "wtext.h"

static struct {
  int mode;                 // Edit, Simu
  RTex2D img_target_tex;    /* Image Drawing target image */
  RTex2D level_overlay_tex; /* Image overlay where the lua stuff can draw to. */
  v2 target_pos;
  Rectangle color_btn[64];
  Rectangle fg_color_rect;
  Color palette[64];
  int num_colors;
  char* fname; /* Currently open filename. */
  Paint ca;
  int header_size;
  int bottom_size;
  // Topbar buttons
  Btn btn_new;
  Btn btn_open;
  Btn btn_save;
  Btn btn_saveas;
  Btn btn_about;
  Btn btn_exit;

  Btn btn_sound;
  Btn btn_sound_paint;
  Btn btn_neon;
  Btn btn_always_on_top;
  Btn btn_wiki;
  Btn btn_pause;
  Btn btn_rewind;
  Btn btn_forward;

  // Tools buttons
  Btn btn_level;
  Btn btn_simu;
  Btn btn_brush;
  Btn btn_text;
  Btn btn_line;
  Btn btn_stamp;
  Btn btn_picker;
  Btn btn_bucket;
  Btn btn_marquee;
  Btn btn_rotate;
  Btn btn_fill;
  Btn btn_fliph;
  Btn btn_flipv;
  Btn btn_sel_open;
  Btn btn_sel_save;
  Btn btn_blueprint_add;
  Btn btn_line_sep;
  Btn btn_line_sep_r;

  Btn btn_layer_push;
  Btn btn_layer_pop;
  Btn btn_layer[MAX_LAYERS];
  Btn btn_layer_v[MAX_LAYERS];

  // speed buttons
  Btn btn_clockopt[6];
  Btn btn_sim_show_t;

  Sim sim;
  HSim hsim;
  bool use_neon;
  bool rewind_pressed;
  bool forward_pressed;
  bool paused;
  double simu_target_steps;
  int pix_toggle;
  bool mouseIsPen;
  Image sidepanel_img;
  Texture2D sidepanel_tex;
  bool looping;
  Sound sound;
  Sound sound_success;
  Sound sound_click;
  Sound sound_click2;
  Sound sound_click3;
  Sound sound_oops;
  Sound sound2;
  float base_volume;
  bool always_on_top;

  bool mute;
  bool muted_paint;
  int clock_speed;

  bool time_open;
  double time_ref;
  char mouse_msg[200];
  int mouse_msg_type;
  bool sim_show_t;

  Rectangle* bot_layout;
  double hover_wire_distance;
  v2 time_c;
  v2 time_pos_ref;
  // lua_State* L; /* Lua context used for configuration */
  Rectangle energy_rect;
  Callback dialog_callback;
} C = {0};

static inline int maxint(int a, int b) { return a > b ? a : b; }

#define MSG_DURATION 2
#define TXT_SIMU_LOOPING "Loop Detected: There are wires updating non-stop..."
#define TXT_CRASH_REASON_MULTIPLE_GATE_CIRCUIT \
  \ "Circuits with multiple input gates.";
#define TXT_CRASH_REASON_MISSING_INPUTS "NANDs must have its 2 inputs filled";
#define TXT_CRASH_REASON_MISSING_OUTPUT \
  "NANDs must be connected to an output wire.";

static void main_update_title();
static void main_update_layout();
static void main_update_viewport();
static void main_draw_status_bar();
static void main_draw_error_message(const char* msg);
static void main_draw_mouse_extra();
static void main_check_file_drop();
static void main_update_widgets();
static void main_update_controls();
static void main_update_hud();
static void main_new_file();
static void main_open_file_modal();
static RectangleInt main_get_target_region();
static void main_open_selection();
static void main_save_selection();
static int main_get_simu_mode() { return C.mode; }
static bool main_is_simulation_on() {
  return main_get_simu_mode() == MODE_SIMU;
}

static void on_modal_before_open() {
  if (C.always_on_top) {
    ClearWindowState(FLAG_WINDOW_TOPMOST);
  }
}
static void on_modal_after_open() {
  if (C.always_on_top) {
    SetWindowState(FLAG_WINDOW_TOPMOST);
  }
}

void main_open() {
  ui_winpush(WINDOW_MAIN);
  easy_blinking_open();
}

void on_paint_act() {
  if (!C.muted_paint) {
    int al = C.ca.h.layer;
    if (!IsSoundPlaying(C.sound_click2)) {
      SetSoundPitch(C.sound_click2, 1 << al);
      PlaySound(C.sound_click2);
    }
  }
}

void on_click() {
  if (!C.mute) PlaySound(C.sound_click);
}

static float get_simu_dt() {
  double seconds_to_frame = 60.0;
  double ticks_per_frame = 4;
  double v1 = ticks_per_frame * seconds_to_frame;
  int c = C.clock_speed;
  switch (c) {
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
      return v1 * 16 * 2;
      break;
    case 5:
      return v1 * 32 * 4;
      break;
  }
  return 10000;
}

static inline int get_simu_speed() {
  if (C.rewind_pressed) return -1;
  if (C.forward_pressed) return 1;
  return (C.paused || C.time_open) ? 0 : 1;
}

static float get_simu_slack_steps() {
  return C.simu_target_steps - C.sim.state.cur_tick;
}

void load_palette(const char* fname) {
  Image pal = LoadImage(fname);
  Color* colors = pal.data;
  int w = pal.width;
  if (w > 64) w = 64;
  C.num_colors = w;
  int c = w / 2;
  for (int i = 0; i < w; i++) {
    int x = i % c;
    int y = i / c;
    C.palette[2 * x + y] = colors[i];
  }
};

void main_init(GameRegistry* registry) {
  srand(time(NULL));
  C.use_neon = true;
  C.sound = LoadSound("../assets/s2.wav");
  C.sound_click = LoadSound("../assets/click.wav");
  C.sound_success = LoadSound("../assets/success.wav");
  C.sound_click2 = LoadSound("../assets/paintact.wav");
  C.sound_click3 = LoadSound("../assets/paintact2.wav");
  C.sound_oops = LoadSound("../assets/oops.wav");
  C.bot_layout = parse_layout("../assets/layout/main_bot_layout.png");
  C.sound2 = LoadSound("../assets/click.wav");
  C.base_volume = .2f;
  C.muted_paint = false;
  SetSoundVolume(C.sound, .1);
  SetSoundVolume(C.sound2, C.base_volume);
  SetSoundVolume(C.sound_click, C.base_volume);
  SetSoundVolume(C.sound_click2, C.base_volume);
  SetSoundVolume(C.sound_click3, C.base_volume);
  win_log_init();
  C.clock_speed = 2;
  C.palette[0] = WHITE;
  C.palette[1] = BLACK;
  C.palette[2] = RED;
  C.palette[3] = GREEN;
  C.palette[4] = BLUE;
  C.palette[5] = YELLOW;
  C.palette[6] = PURPLE;
  C.palette[7] = SKYBLUE;
  C.palette[8] = DARKGREEN;
  C.palette[9] = PINK;
  C.palette[10] = ORANGE;
  C.palette[11] = DARKPURPLE;
  C.palette[12] = VIOLET;
  C.palette[13] = BEIGE;
  C.palette[14] = DARKBLUE;
  C.palette[15] = LIME;
  C.palette[16] = GetColor(0x111111FF);
  C.palette[17] = GetColor(0x333333FF);

  C.num_colors = 18;
  load_palette("../assets/pal.png");
  int s = ui_get_scale();
  C.header_size = 24 * s;
  C.bottom_size = 3 * 17 * 1 * s - 6 * s;
  paint_init(&C.ca);
  paint_set_color(&C.ca, C.palette[3]);
  if (false) {
    // Image img = LoadImage("../solutions/big2.png");
    Image img = LoadImage("../a.png");
    paint_load_image(&C.ca, img);
  } else {
    if (ui_is_demo()) {
      Image img = LoadImage("../assets/help_small2.png");
      paint_load_image(&C.ca, img);
    } else {
      Image img = LoadImage("../assets/help.png");
      paint_load_image(&C.ca, img);
    }
    // paint_new_buffer(&C.ca);
  }
  C.sidepanel_img = gen_image_simple(200, 800);
  C.sidepanel_tex = LoadTextureFromImage(C.sidepanel_img);
  main_update_viewport();
  main_update_title();
  main_update_widgets();
  level_load_default();
  sim_dry_run(registry);
}

static void simu_play_sounds() {
  int na = arrlen(C.sim.ui_events);
  int r = 0;
  if (!C.mute) {
    for (int i = 0; i < na; i++) {
      SimUiEvent ev = C.sim.ui_events[i];
      if (ev.sound > 0) {
        // SetSoundPitch(C.sound, ev.sound);
        PlaySound(C.sound);
      }
    }
  }
}

/*
 * Runs simulation to ensure slack step is always 0 <= slack_step < 1
 */
static void main_update_simu() {
  profiler_tic("Simulation");
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

  while (slack_steps < 0.f) {
    /* Can't go backward */
    if (!hsim_has_prv(&C.hsim)) {
      slack_steps = 0.f;
      C.simu_target_steps = C.sim.state.cur_tick;
      break;
    }
    hsim_prv(&C.hsim);
    slack_steps += 1.f;
  }

  while (slack_steps >= 1.f) {
    hsim_nxt(&C.hsim);
    simu_play_sounds();
    int completed_at = C.sim.level_complete_dispatched_at;
    /* pauses one tick after completion */
    if (completed_at >= 0 && completed_at == C.sim.state.cur_tick - 1) {
      C.paused = true;
      C.simu_target_steps = C.sim.state.cur_tick;
      on_click();
      break;
    }
    slack_steps -= 1.f;
  }

  profiler_tac();
  // assert(get_simu_slack_steps() >= 0 && get_simu_slack_steps() < 1.f);
}

float get_clock_delta(v2 c, v2 a, v2 b) {
  float ay = a.y - c.y;
  float ax = a.x - c.x;
  float by = b.y - c.y;
  float bx = b.x - c.x;

  float ra = sqrt(ax * ax + ay * ay + 1e-1);
  float rb = sqrt(bx * bx + by * by + 1e-1);

  float cross = -ax * by + ay * bx;
  return (180 / M_PI) * asin(cross / (ra * rb));
  // a cross b
  // [ax, ay, az]
  // [bx, by, bz]
  // [0, 0, 1]
}

static void draw_pin_sockets(RenderTexture target) {
  BeginTextureMode(target);
  Cam2D cam = C.ca.cam;
  rlPushMatrix();
  rlTranslatef(cam.off.x, cam.off.y, 0);
  rlScalef(cam.sp, cam.sp, 1);
  PinGroup* l_pg = getlevel()->pg;
  for (int ig = 0; ig < arrlen(l_pg); ig++) {
    PinGroup* pg = &l_pg[ig];
    int w = C.ca.h.buffer[0].width;
    int npin = arrlen(pg->pins);
    for (int ipin = 0; ipin < npin; ipin++) {
      int x = pg->pins[ipin].x;
      int y = pg->pins[ipin].y;
      int w = paint_img_width(&C.ca);
      int h = paint_img_height(&C.ca);
      if (x < 0) x = w + x;
      if (y < 0) y = h + y;
      Rectangle source;
      Texture sprite = ui_get_sprites();
      Rectangle target = {x, y, 1, 1};
      int type = pg->type;
      Color clr;
      if (type == PIN_LUA2IMG) {
        clr = GREEN;
        source = (Rectangle){576, 160, 16, 16};
      } else {
        clr = RED;
        source = (Rectangle){576, 160 + 16, 16, 16};
      }
      if (cam.sp < 16) {
        source = (Rectangle){576 + 16, 160, 16, 16};
      }
      DrawTexturePro(sprite, source, target, (Vector2){0, 0}, 0, clr);
    }
  }
  rlPopMatrix();
  EndTextureMode();
}

void main_update() {
  main_update_viewport();
  C.rewind_pressed = false;
  C.forward_pressed = false;
  main_check_file_drop();
  msg_update();
  main_update_hud();
  main_update_controls();
  main_update_widgets();
  profiler_tic("simu");
  if (main_get_simu_mode() == MODE_SIMU) {
    double dt = get_simu_dt();
    if (C.time_open) {
      v2 pos = GetMousePosition();
      v2 ref = C.time_pos_ref;
      double diff = 0.003 * dt * get_clock_delta(C.time_c, pos, ref);
      double tgt = C.time_ref + diff;
      if (tgt < 0) {
        C.time_ref += tgt;
        tgt = 0;
      }
      C.simu_target_steps = tgt;
      C.time_ref = tgt;
      C.time_pos_ref = pos;
    } else {
      double frame_time = ui_get_frame_time();
      int dir = get_simu_speed();
      if (dir != 0) {
        double new_time = C.simu_target_steps + frame_time * dir * dt;
        C.simu_target_steps = new_time >= 0 ? new_time : 0;
      }
    }

    main_update_simu();
  }
  profiler_tac();
  profiler_tic("Rendering");

  int mode = main_get_simu_mode();
  if (mode == MODE_EDIT) {
    paint_render_texture(&C.ca, C.sidepanel_tex, C.img_target_tex);
    draw_pin_sockets(C.img_target_tex);
  }

  if (mode == MODE_SIMU || mode == MODE_ERROR) {
    float slack_steps = get_simu_slack_steps();
    Texture texs[MAX_LAYERS] = {0};
    int nl = hist_get_num_layers(&C.ca.h);
    for (int i = 0; i < nl; i++) {
      texs[i] = C.ca.h.t_buffer[i].texture;
    }
    int tw = C.img_target_tex.texture.width;
    int th = C.img_target_tex.texture.height;
    float dt = get_simu_dt();
    float frame_steps = dt * 1.0 / 60.0;
    if (C.paused) {
      frame_steps = 0;
    }
    profiler_tic("sim_render");
    int hide_mask = 0;
    for (int i = 0; i < nl; i++) {
      if (C.ca.hidden[i]) {
        hide_mask = hide_mask | (1 << i);
      }
    }

    Tex* rendered = sim_render_v2(&C.sim, tw, th, C.ca.cam, frame_steps,
                                  slack_steps, hide_mask, C.use_neon);
    profiler_tac();

    BeginTextureMode(C.img_target_tex);
    ClearBackground(PURPLE);
    EndTextureMode();

    texdraw2(C.img_target_tex, rendered->rt);

    texdel(rendered);
  }

  profiler_tac();
  BeginTextureMode(C.level_overlay_tex);
  ClearBackground(BLANK);

  rlPushMatrix();
  rlTranslatef(C.ca.cam.off.x, C.ca.cam.off.y, 0);
  rlScalef(C.ca.cam.sp, C.ca.cam.sp, 1);
  level_draw_board(getlevel()); /* drawboard() is called every frame */
  rlPopMatrix();

  if (main_is_simulation_on()) { /* only calls draw() when simu is on */
    rlPushMatrix();
    level_draw(getlevel());
    rlPopMatrix();
  }

  EndTextureMode();
  texcleanup();
}

static void draw_resize_demo_message() {
  if (C.ca.resizeHovered || C.ca.resizePressed) {
    rlPushMatrix();
    Vector2 mouse = GetMousePosition();
    rlTranslatef(mouse.x, mouse.y - 20, 0);
    rlScalef(2, 2, 1);
    if (ui_is_demo()) {
      const char msg1[] = "Max image size in demo version is 64x64";
      const char msg2[] = "Full version available on Steam.";
      rlTranslatef(0, -14, 0);
      font_draw_texture(msg2, 2, 2, BLACK);
      font_draw_texture(msg2, 0, 0, RED);
      rlTranslatef(0, -14, 0);
      font_draw_texture(msg1, 2, 2, BLACK);
      font_draw_texture(msg1, 0, 0, RED);
    }
  }
  rlPopMatrix();
}

static void main_update_paint_cursor_type() {
  if (C.mode == MODE_EDIT) {
    tool_t tool = paint_get_display_tool(&C.ca);
    MouseCursorType cursor = MOUSE_ARROW;
    switch (tool) {
      case TOOL_SEL: {
        bool move = paint_get_mouse_over_sel(&C.ca) ||
                    paint_get_is_tool_sel_moving(&C.ca);
        if (move) {
          cursor = MOUSE_MOVE;
        } else {
          cursor = MOUSE_SELECTION;
        }
        break;
      };
      case TOOL_BRUSH:
      case TOOL_LINE: {
        cursor = MOUSE_PEN;
        C.mouseIsPen = true;
        break;
      }
      case TOOL_BUCKET: {
        cursor = MOUSE_BUCKET;
        break;
      }
      case TOOL_PICKER: {
        cursor = MOUSE_PICKER;
        break;
      }
      default: {
        cursor = MOUSE_ARROW;
        break;
      }
    }
    if (C.ca.resizeHovered || C.ca.resizePressed) {
      cursor = MOUSE_RESIZE;
    }
    ui_set_cursor(cursor);
  } else {
    // int cs;
    // Paint_GetSimuPixelToggleState(&C.ca, &cs);
    // if (cs == 0 || cs == 1) {
    //   ui->cursor = MOUSE_POINTER;
    // } else {
    //   ui->cursor = MOUSE_ARROW;
    // }
  }
}

void main_start_simu() {
  assert(main_get_simu_mode() != MODE_SIMU);
  C.time_open = false;
  C.looping = false;
  C.paused = false;
  C.rewind_pressed = false;
  C.forward_pressed = false;
  paint_pause(&C.ca);

  RenderTexture2D texs[MAX_LAYERS];
  Image imgs[MAX_LAYERS];
  int nl = hist_get_num_layers(&C.ca.h);
  for (int i = 0; i < nl; i++) {
    imgs[i] = C.ca.h.buffer[i];
    texs[i] = C.ca.h.t_buffer[i];
  }
  sim_init(&C.sim, nl, imgs, getlevel(), &texs[0]);
  C.hsim = wrap_sim(&C.sim);
  C.simu_target_steps = 0;
  C.pix_toggle = -1;
  if (sim_has_errors(&C.sim)) {
    if (!C.mute) {
      PlaySound(C.sound_oops);
    }
    C.mode = MODE_ERROR;
    return;
  }
  C.mode = MODE_SIMU;
}

void main_stop_simu() {
  // assert(main_get_simu_mode() != MODE_EDIT);
  assert(main_get_simu_mode() == MODE_SIMU ||
         main_get_simu_mode() == MODE_ERROR);
  hsim_destroy(&C.hsim);
  sim_destroy(&C.sim);

  C.mode = MODE_EDIT;
  C.time_open = false;
  C.rewind_pressed = false;
  C.forward_pressed = false;
}

static void main_toggle_simu() {
  int mode = C.mode;
  if (mode == MODE_EDIT) {
    main_start_simu();
  } else {
    main_stop_simu();
  }
}

static void add_blueprint() {
  if (!paint_get_has_selection(&C.ca)) {
    return;
  }

  Image full = paint_export_sel(&C.ca);
  int nl = hist_get_num_layers_sel(&C.ca.h);
  int istamp = stamp_create_and_open(nl, C.ca.h.selbuffer, full);
  if (istamp == -1) {
    msg_add(
        "INVENTORY FULL: Failed to create blueprint. Please open inventory "
        "space.",
        3);
  }
  UnloadImage(full);
}

static void on_paste_text(void* ctx, const char* txt) { main_paste_text(txt); }

static void on_open_click() {
  main_ask_for_save_and_proceed(main_open_file_modal);
}

static void on_new_click() { main_ask_for_save_and_proceed(main_new_file); }

void main_update_controls() {
  if (is_control_down()) {
    if (IsKeyPressed(KEY_S)) {
      main_on_save_click(false);
    }
    if (IsKeyPressed(KEY_O)) on_open_click();
    if (IsKeyPressed(KEY_N)) on_new_click();
  }
  paint_update_pixel_position(&C.ca);
  paint_enforce_mouse_on_image_if_need(&C.ca);
  ui_set_cursor(MOUSE_ARROW);
  Paint* pnt = &C.ca;
  bool isEdit = main_get_simu_mode() == MODE_EDIT;
  bool mouseOnTarget = C.ca.mouseOnTarget;
  C.mouseIsPen = false;
  C.mouse_msg[0] = '\0';
  C.mouse_msg_type = -1;
  bool paint_hit = mouseOnTarget && ui_get_hit_count() == 0;
  if (mouseOnTarget) {
    paint_handle_wheel_zoom(pnt);
    paint_handle_camera_movement(pnt);
    if (paint_hit) {
      main_update_paint_cursor_type();
    }
  }

  if (isEdit && IsKeyPressed(KEY_TAB) && !IsKeyDown(KEY_LEFT_ALT)) {
    paint_layer_alt(&C.ca);
  }

  if (isEdit && IsKeyPressed(KEY_Q)) {
    // win_blueprint_open();
    win_stamp_open();
  }

  if (isEdit && IsKeyPressed(KEY_F1)) {
    on_click();
    paint_set_layer(&C.ca, 0);
  }

  if (isEdit && IsKeyPressed(KEY_F2)) {
    if (paint_get_num_layers(&C.ca) > 1) {
      on_click();
      paint_set_layer(&C.ca, 1);
    }
  }

  if (isEdit && IsKeyPressed(KEY_F3)) {
    if (paint_get_num_layers(&C.ca) > 2) {
      on_click();
      paint_set_layer(&C.ca, 2);
    }
  }

  if (isEdit && IsKeyPressed(KEY_T)) {
    text_modal_open(on_paste_text, NULL, NULL);
  }
  if (isEdit) paint_handle_mouse(pnt, paint_hit);
  if (!isEdit && ui_get_hit_count() == 0) {
    if (mouseOnTarget) {
      int bw = pnt->h.buffer[0].width;
      int bh = pnt->h.buffer[0].height;
      v2i crs = pnt->pixelCursor;
      if (crs.x >= 0 && crs.x < bw && crs.y >= 0 && crs.y < bh) {
        int searchRadius = 5;
        int pix_toggle;
        sim_find_nearest_pixel(&C.sim, searchRadius, pnt->fPixelCursor,
                               &pix_toggle);
        C.hover_wire_distance = -1;
        if (pix_toggle != -1) {
          bool error_mode = sim_has_errors(&C.sim);
          ui_set_cursor(MOUSE_POINTER);
          if (!error_mode) {
            C.hover_wire_distance = sim_get_pixel_dist(&C.sim, pix_toggle);
            if (C.sim_show_t) {
              C.mouse_msg_type = 0;
              strcpy(C.mouse_msg, TextFormat("T=%.2f", C.hover_wire_distance));
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
              C.pix_toggle = pix_toggle;
              C.paused = false;
            }
          } else {
            int status = sim_get_pixel_error_status(&C.sim, pix_toggle);
            if (status == STATUS_TOOSLOW) {
              C.mouse_msg_type = 1;
              strcpy(C.mouse_msg, "Wire too slow.");
            }
            if (status == STATUS_CONFLICT) {
              C.mouse_msg_type = 1;
              strcpy(C.mouse_msg, "More than one nand connected to the wire.");
            }
          }
        }
      }
    }
  }
  if (IsKeyPressed(KEY_SPACE)) {
    main_toggle_simu();
  }
  int nl = paint_get_num_layers(&C.ca);
  int l = hist_get_active_layer(&C.ca.h);
  if (isEdit && IsKeyPressed(KEY_LEFT_BRACKET)) {
    if (l > 0) {
      paint_set_layer(&C.ca, l - 1);
    }
  }
  if (isEdit && IsKeyPressed(KEY_RIGHT_BRACKET)) {
    if (l < nl - 1) {
      paint_set_layer(&C.ca, l + 1);
    }
  }

  if (isEdit && IsKeyPressed(KEY_U) && paint_get_has_selection(&C.ca)) {
    add_blueprint();
  }

  if (!isEdit) {
    if (IsKeyPressed(KEY_K)) {
      C.paused = !C.paused;
    }
    if (IsKeyDown(KEY_J)) C.rewind_pressed = true;
    if (IsKeyDown(KEY_L)) C.forward_pressed = true;
  }

  if (C.mode == MODE_SIMU) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      C.time_open = true;
      C.time_ref = C.simu_target_steps;  // C.sim.state.cur_tick;
      C.time_pos_ref = GetMousePosition();
      RectangleInt reg = main_get_target_region();
      // C.time_c = C.time_pos_ref;
      C.time_c = (v2){
          reg.x + reg.width / 2.0,
          reg.y + reg.height / 2.0,
      };
      // C.time_c = (v2){
      //     C.time_pos_ref.x,
      //     C.time_pos_ref.y + 50,
      // };
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
      C.time_open = false;
    }
  }

  paint_movement_keys(&C.ca);
  if (isEdit) paint_handle_keys(&C.ca);
}

void main_open_selection() {
  on_modal_before_open();
  ModalResult mr = modal_open_file(NULL);
  on_modal_after_open();
  if (mr.ok) {
    main_paste_file(mr.fPath, 0);
    free(mr.fPath);
  } else if (mr.cancel) {
  } else {
    char txt[500];
    snprintf(txt, sizeof(txt), "ERROR: %s\n", mr.errMsg);
    msg_add(txt, MSG_DURATION);
  }
}

void main_save_selection() {
  on_modal_before_open();
  ModalResult mr = modal_save_file(NULL, NULL);
  on_modal_after_open();
  if (mr.cancel) {
    return;
  } else if (!mr.ok) {
    char txt[500];
    snprintf(txt, sizeof(txt), "ERROR: %s\n", mr.errMsg);
    msg_add(txt, MSG_DURATION);
    return;
  }

  if (mr.fPath && mr.ok) {
    Image out = paint_export_sel(&C.ca);
    // Before saving, add black pixels back
    if (!ExportImage(out, mr.fPath)) {
      msg_add("ERROR: Could not save selection ...", MSG_DURATION);
      return;
    }
    UnloadImage(out);
    msg_add("Selection Image Saved.", MSG_DURATION);
    return;
  }
}

static void toggle_simu_pause() { C.paused = !C.paused; }
static void toggle_sound() { C.mute = !C.mute; }
static void toggle_neon() { C.use_neon = !C.use_neon; }
static void toggle_sound_paint() { C.muted_paint = !C.muted_paint; }

static void toggle_always_on_top() {
  C.always_on_top = !C.always_on_top;
  if (C.always_on_top) {
    SetWindowState(FLAG_WINDOW_TOPMOST);
  } else {
    ClearWindowState(FLAG_WINDOW_TOPMOST);
  }
}

static void toggle_sim_show_t() { C.sim_show_t = !C.sim_show_t; }

void main_update_hud() {
  Paint* ca = &C.ca;
  if (btn_update(&C.btn_new)) on_new_click();
  if (btn_update(&C.btn_open)) on_open_click();
  if (btn_update(&C.btn_save)) main_on_save_click(false);
  if (btn_update(&C.btn_saveas)) main_on_save_click(true);
  if (btn_update(&C.btn_about)) easy_about_open();
  if (btn_update(&C.btn_exit)) ui_set_close_requested();
  if (btn_update(&C.btn_sound)) toggle_sound();
  if (btn_update(&C.btn_neon)) toggle_neon();
  if (btn_update(&C.btn_sound_paint)) toggle_sound_paint();
  if (btn_update(&C.btn_always_on_top)) toggle_always_on_top();

  if (btn_update(&C.btn_layer_pop)) paint_layer_pop(&C.ca);
  if (btn_update(&C.btn_layer_push)) paint_layer_push(&C.ca);
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (btn_update(&C.btn_layer[l])) paint_set_layer(&C.ca, l);
    if (btn_update(&C.btn_layer_v[l])) {
      paint_set_layer_vis(&C.ca, l, !paint_get_layer_vis(&C.ca, l));
    }
  }

  if (btn_update(&C.btn_sel_open)) main_open_selection();
  if (btn_update(&C.btn_sel_save)) main_save_selection();
  if (btn_update(&C.btn_blueprint_add)) add_blueprint();

  if (btn_update(&C.btn_line)) paint_set_tool(ca, TOOL_LINE);
  if (btn_update(&C.btn_brush)) paint_set_tool(ca, TOOL_BRUSH);
  if (btn_update(&C.btn_marquee)) paint_set_tool(ca, TOOL_SEL);
  if (btn_update(&C.btn_text)) text_modal_open(on_paste_text, NULL, NULL);
  if (btn_update(&C.btn_bucket)) paint_set_tool(ca, TOOL_BUCKET);
  if (btn_update(&C.btn_picker)) paint_set_tool(ca, TOOL_PICKER);

  if (btn_update(&C.btn_rotate)) paint_act_sel_rot(ca);
  if (btn_update(&C.btn_fliph)) paint_act_sel_fliph(ca);
  if (btn_update(&C.btn_flipv)) paint_act_sel_flipv(ca);
  if (btn_update(&C.btn_fill)) paint_act_sel_fill(ca);

  if (btn_update(&C.btn_line_sep)) number_modal_open();
  if (btn_update(&C.btn_line_sep_r)) main_set_line_sep(1);

  if (btn_update(&C.btn_simu)) main_toggle_simu();
  if (btn_update(&C.btn_pause)) toggle_simu_pause();
  btn_update(&C.btn_rewind);
  btn_update(&C.btn_forward);
  if (C.btn_rewind.pressed) C.rewind_pressed = true;
  if (C.btn_forward.pressed) C.forward_pressed = true;

  if (btn_update(&C.btn_level)) win_level_open();
  if (btn_update(&C.btn_wiki)) tutorial_open();
  if (btn_update(&C.btn_stamp)) win_stamp_open();

  if (btn_update(&C.btn_clockopt[0])) C.clock_speed = 0;
  if (btn_update(&C.btn_clockopt[1])) C.clock_speed = 1;
  if (btn_update(&C.btn_clockopt[2])) C.clock_speed = 2;
  if (btn_update(&C.btn_clockopt[3])) C.clock_speed = 3;
  if (btn_update(&C.btn_clockopt[4])) C.clock_speed = 4;
  if (btn_update(&C.btn_clockopt[5])) C.clock_speed = 5;
  if (btn_update(&C.btn_sim_show_t)) toggle_sim_show_t();

  Vector2 pos = GetMousePosition();
  if (rect_hover(C.fg_color_rect, pos)) {
    ui_inc_hit_count();
  }
  for (int i = 0; i < C.num_colors; i++) {
    if (rect_hover(C.color_btn[i], pos) && ui_get_hit_count() == 0) {
      ui_inc_hit_count();
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        on_click();
        paint_set_color(&C.ca, C.palette[i]);
      }
    }
  }
}

void main_draw() {
  ClearBackground(BLANK);
  draw_default_tiled_screen();
  // Draws target
  int th = C.img_target_tex.texture.height;
  int tw = C.img_target_tex.texture.width;
  Vector2 rpos = C.target_pos;
  DrawRectangle(rpos.x, rpos.y, tw, th, BLACK);
  draw_rt_on_screen(C.img_target_tex, rpos);
  draw_rt_on_screen(C.level_overlay_tex, rpos);

  Rectangle inner_content = {
      C.target_pos.x,
      C.target_pos.y,
      C.level_overlay_tex.texture.width,
      C.level_overlay_tex.texture.height,
  };
  draw_default_tiled_frame(inner_content);
  main_draw_status_bar();

  if (C.time_open) {
    RectangleInt reg = main_get_target_region();
    v2 c = {
        reg.x + reg.width / 2.0,
        reg.y + reg.height / 2.0,
    };
    v2 pos = GetMousePosition();

    Tex* tmp = texnew(reg.width, reg.height);
    texclear(tmp, BLANK);
    float mx = pos.x - c.x;
    float my = pos.y - c.y;
    texclock(tmp, mx, my);
    ui_set_cursor(MOUSE_CLOCK);
    draw_rt_on_screen(tmp->rt, rpos);
    texdel(tmp);
  }

  Texture2D sprites = ui_get_sprites();
  int bscale = ui_get_scale();

  btn_draw_text(&C.btn_layer_push, bscale, "+");
  btn_draw_text(&C.btn_layer_pop, bscale, "-");
  btn_draw_text(&C.btn_layer[0], bscale, "F1");
  btn_draw_text(&C.btn_layer[1], bscale, "F2");
  btn_draw_text(&C.btn_layer[2], bscale, "F3");

  int al = hist_get_active_layer(&C.ca.h);
  int v0 = paint_get_layer_vis(&C.ca, 0);
  int v1 = paint_get_layer_vis(&C.ca, 1);
  int v2 = paint_get_layer_vis(&C.ca, 2);
  int v3 = paint_get_layer_vis(&C.ca, 3);
  btn_draw_icon(&C.btn_layer_v[0], bscale, sprites,
                v0 ? rect_eye_on : rect_eye_off);
  btn_draw_icon(&C.btn_layer_v[1], bscale, sprites,
                v1 ? rect_eye_on : rect_eye_off);
  btn_draw_icon(&C.btn_layer_v[2], bscale, sprites,
                v2 ? rect_eye_on : rect_eye_off);
  btn_draw_icon(&C.btn_layer_v[3], bscale, sprites,
                v3 ? rect_eye_on : rect_eye_off);

  btn_draw_icon(&C.btn_new, bscale, sprites, rect_new);
  btn_draw_icon(&C.btn_open, bscale, sprites, rect_open);
  btn_draw_icon(&C.btn_save, bscale, sprites, rect_save);
  btn_draw_icon(&C.btn_saveas, bscale, sprites, rect_saveas);
  btn_draw_icon(&C.btn_about, bscale, sprites, rect_info);
  btn_draw_icon(&C.btn_exit, bscale, sprites, rect_exit);

  btn_draw_text(&C.btn_sound, bscale,
                TextFormat("Circuit Sound %s", C.mute ? "Off" : "On"));
  btn_draw_text(&C.btn_neon, bscale,
                TextFormat("Neon %s", C.use_neon ? "On" : "Off"));
  btn_draw_text(&C.btn_sound_paint, bscale,
                TextFormat("Paint Sound %s", C.muted_paint ? "Off" : "On"));
  btn_draw_text(&C.btn_always_on_top, bscale, "Always on Top");

  btn_draw_icon(&C.btn_clockopt[0], bscale, sprites, rect_hz0);
  btn_draw_icon(&C.btn_clockopt[1], bscale, sprites, rect_hz1);
  btn_draw_icon(&C.btn_clockopt[2], bscale, sprites, rect_hz4);
  btn_draw_icon(&C.btn_clockopt[3], bscale, sprites, rect_hz16);
  btn_draw_icon(&C.btn_clockopt[4], bscale, sprites, rect_hz64);
  btn_draw_icon(&C.btn_clockopt[5], bscale, sprites, rect_hz1k);
  btn_draw_icon(&C.btn_sim_show_t, bscale, sprites, rect_inspect_wire);

  int mode = main_get_simu_mode();
  bool simu_on = mode == MODE_SIMU || mode == MODE_ERROR;
  btn_draw_icon(&C.btn_simu, bscale, sprites, simu_on ? rect_stop : rect_start);
  btn_draw_icon(&C.btn_rewind, bscale, sprites, rect_rewind);
  btn_draw_icon(&C.btn_forward, bscale, sprites, rect_forward);
  btn_draw_icon(&C.btn_pause, bscale, sprites, rect_pause);

  btn_draw_text(&C.btn_wiki, bscale, "Wiki");
  if (getlevel()) {
    btn_draw_text(&C.btn_level, bscale,
                  TextFormat("Level: %s", getlevel()->ldef->name));
  }

  bool color_disabled = mode != MODE_EDIT;
  btn_draw_color(C.fg_color_rect, paint_get_color(&C.ca), false,
                 color_disabled);
  for (int i = 0; i < C.num_colors; i++) {
    btn_draw_color(C.color_btn[i], C.palette[i],
                   COLOR_EQ(C.palette[i], paint_get_color(&C.ca)),
                   color_disabled);
  }
  btn_draw_icon(&C.btn_line, bscale, sprites, rect_line);
  btn_draw_icon(&C.btn_stamp, bscale, sprites, rect_blueprint);
  btn_draw_icon(&C.btn_brush, bscale, sprites, rect_brush);
  btn_draw_icon(&C.btn_picker, bscale, sprites, rect_picker);
  btn_draw_icon(&C.btn_bucket, bscale, sprites, rect_bucket);
  btn_draw_icon(&C.btn_marquee, bscale, sprites, rect_marquee);
  btn_draw_icon(&C.btn_text, bscale, sprites, rect_text);
  btn_draw_icon(&C.btn_rotate, bscale, sprites, rect_rot);
  btn_draw_icon(&C.btn_fliph, bscale, sprites, rect_fliph);
  btn_draw_icon(&C.btn_flipv, bscale, sprites, rect_flipv);
  btn_draw_icon(&C.btn_fill, bscale, sprites, rect_fill);
  btn_draw_icon(&C.btn_line_sep, bscale, sprites, rect_line_sep);
  btn_draw_icon(&C.btn_line_sep_r, bscale, sprites, rect_line_sep_r);
  btn_draw_icon(&C.btn_sel_open, bscale, sprites, rect_sel_open);
  btn_draw_icon(&C.btn_sel_save, bscale, sprites, rect_sel_save);
  btn_draw_icon(&C.btn_blueprint_add, bscale, sprites, rect_blueprint_add);

  msg_draw();

  // We only draw the legends if this window is the active window.
  if (ui_get_window() == WINDOW_MAIN) {
    btn_draw_legend(&C.btn_new, bscale, "New Image (CTRL+N)");

    if (ui_is_demo()) {
      btn_draw_legend(&C.btn_open, bscale,
                      "Load Image \n`(Not Available in Demo)`");
    } else {
      btn_draw_legend(&C.btn_open, bscale, "Load Image (CTRL+O)");
    }

    btn_draw_legend(&C.btn_save, bscale, "Save Image (CTRL+S)");
    btn_draw_legend(&C.btn_saveas, bscale, "Save Image As .. ");

    btn_draw_legend(&C.btn_stamp, bscale,
                    "Open Blueprints Window (Q)\nOpens even if selection tool "
                    "is not active.");
    btn_draw_legend(&C.btn_about, bscale, "About Circuit Artist");
    btn_draw_legend(&C.btn_exit, bscale, "Exit");

    btn_draw_legend(&C.btn_layer_push, bscale, "Add Top Layer (max 3)");
    btn_draw_legend(&C.btn_layer_pop, bscale, "Remove Top Layer");

    for (int i = 0; i < MAX_LAYERS; i++) {
      btn_draw_legend(&C.btn_layer_v[i], bscale,
                      TextFormat("Show/Hide layer %d.", i + 1));
    }

    btn_draw_legend(
        &C.btn_layer[0], bscale,
        "Bottom layer (F1)\nNANDs are allowed in this layer.\nUse "
        "(TAB) to quickly alternate between previously used layer.");
    btn_draw_legend(
        &C.btn_layer[1], bscale,
        "Second layer (F2)\nNANDs are NOT allowed in this "
        "layer.\nWires propagate faster here.\nUse (TAB) to quickly "
        "alternate between previously used layer.");
    btn_draw_legend(
        &C.btn_layer[2], bscale,
        "Third layer (F3)\nNANDs are NOT allowed in this layer.\nWires "
        "propagate even faster here.\nUse (TAB) to quickly alternate "
        "between previously used layer.");

    btn_draw_legend(&C.btn_level, bscale, "Select Level/Campaign.");
    btn_draw_legend(&C.btn_wiki, bscale, "Wiki");
    btn_draw_legend(&C.btn_sound, bscale, "Toggle simulation sound");
    btn_draw_legend(&C.btn_neon, bscale,
                    "Toggle neon (glow) during simulation");
    btn_draw_legend(&C.btn_sound_paint, bscale, "Toggle paint sound");
    btn_draw_legend(&C.btn_always_on_top, bscale, "Toggle Always On Top");
    btn_draw_legend(&C.btn_sel_open, bscale, "Load Selection from Image");
    btn_draw_legend(&C.btn_sel_save, bscale, "Save Selection as Image");
    btn_draw_legend(&C.btn_blueprint_add, bscale, "Create blueprint (U)");

    btn_draw_legend(&C.btn_clockopt[0], bscale, "Speed 1");
    btn_draw_legend(&C.btn_clockopt[1], bscale, "Speed 2");
    btn_draw_legend(&C.btn_clockopt[2], bscale, "Speed 3");
    btn_draw_legend(&C.btn_clockopt[3], bscale, "Speed 4");
    btn_draw_legend(&C.btn_clockopt[4], bscale, "Speed 5");
    btn_draw_legend(&C.btn_clockopt[5], bscale, "Speed 6");
    btn_draw_legend(
        &C.btn_sim_show_t, bscale,
        "Show stats on cursor\n`T` = Time it takes to propagate to pixel");

    btn_draw_legend(
        &C.btn_simu, bscale,
        simu_on ? "Stop Simulation (SPACE)" : "Start Simulation (SPACE)");
    btn_draw_legend(&C.btn_pause, bscale, "Pause/Unpause Simulation (K).");
    btn_draw_legend(&C.btn_rewind, bscale,
                    "Rewinds simulation (J).\nYou can also press (RIGHT MOUSE "
                    "BUTTON) during simulation to rewind with more accuracy.");
    btn_draw_legend(&C.btn_forward, bscale,
                    "Forwards simulation (L).\nYou can also press (RIGHT MOUSE "
                    "BUTTON) during simulation to rewind with more accuracy.");
    btn_draw_legend(&C.btn_brush, bscale,
                    "Brush tool (B)\nLeft mouse button: draw\nRight mouse "
                    "button: erase\nPress (ALT) to pick color.");
    btn_draw_legend(&C.btn_line, bscale,
                    "Line tool (L)\n"
                    "Type (NUMBER) to change line size.\n"
                    "Press (SHIFT) while drawing to add corner to start of "
                    "line.\nPress (CTRL) while drawing to add corner to end of "
                    "line.\nLeft mouse button: Draw\nRight mouse button: "
                    "Erase\nPress (ALT) to pick color.");
    btn_draw_legend(
        &C.btn_bucket, bscale,
        "Wire Fill (G)\nLeft mouse button: regular color "
        "fill.\nRight mouse button: black color fill (erase).\nPress (SHIFT) "
        "to force color on overlapping wire.\nPress "
        "(ALT) to pick color.");
    btn_draw_legend(&C.btn_picker, bscale, "Color Picker (I)");
    btn_draw_legend(
        &C.btn_marquee, bscale,
        "Single and Multi-Layer Selection tool (M)\nHold (LEFT CTRL) when "
        "selecting to include all "
        "layers below in selection.\nHold (LEFT CTRL) and drag to create a "
        "copy of "
        "the "
        "selection.\nHold (SHIFT) while dragging to force a strict X or Y "
        "movement.\nUse (ARROW)s to move a selection by single pixels. "
        "(CTRL+ARROW)s to move by 4 pixels. \n(C-C)/(C-V) for copy/paste "
        "selection (copies/pastes to/from clipboard).\n(DELETE) or "
        "(BACKSPACE) "
        "to delete selection.\n (ESCAPE) to deselect.");
    btn_draw_legend(&C.btn_text, bscale, "Add Text (T)");
    btn_draw_legend(&C.btn_fliph, bscale, "Flip selection horizontally (H)");
    btn_draw_legend(&C.btn_flipv, bscale, "Flip selection vertically (V)");
    btn_draw_legend(&C.btn_rotate, bscale, "Rotate selection (R)");
    btn_draw_legend(&C.btn_fill, bscale, "Fill selection (F)");
    btn_draw_legend(&C.btn_line_sep, bscale, "Define line separation width");
    btn_draw_legend(&C.btn_line_sep_r, bscale,
                    "Reset line separation width to 1");
  }

  if (ui_get_window() == WINDOW_MAIN) {
    main_draw_mouse_extra();
    if (ui_is_demo()) {
      draw_resize_demo_message();
    }
  }
}

static Rectangle fix_bottom(int sh, Rectangle r) {
  int rw = 540;
  r.y = r.y + 2 * (sh - rw);
  return r;
}

static Rectangle rgrow(int p, Rectangle r) {
  r.y -= p;
  r.x -= p;
  r.width += 2 * p;
  r.height += 2 * p;
  return r;
}

void main_update_layout() {
  int s = ui_get_scale();
  int sw = s * (GetScreenWidth() / s);
  {
    int y0 = 4 * s;
    int x0 = 4 * s;

    int x1 = x0 + 18 * s;
    int x2 = x1 + 18 * s;
    int x3 = x2 + 18 * s;
    int x4 = x3 + 18 * s;
    int x5 = x4 + 18 * s;
    int x6 = x5 + 18 * s;
    int x7 = x6 + 18 * s;
    int x8 = x7 + 18 * s;
    int x9 = x8 + 18 * s;
    int x10 = x9 + 18 * s;
    int x11 = x10 + 18 * s;
    int x12 = x11 + 18 * s;
    int x13 = x12 + 18 * s;
    int bw = 17 * s;
    int bh = 17 * s;

    C.btn_new.hitbox = (Rectangle){x0, y0, bw, bh};
    C.btn_open.hitbox = (Rectangle){x1, y0, bw, bh};
    C.btn_save.hitbox = (Rectangle){x2, y0, bw, bh};
    C.btn_saveas.hitbox = (Rectangle){x3, y0, bw, bh};
    C.btn_about.hitbox = (Rectangle){x4, y0, bw, bh};
    C.btn_exit.hitbox = (Rectangle){x5, y0, bw, bh};

    C.btn_level.hitbox = (Rectangle){x7, y0, 9 * bw, bh};
    x7 += C.btn_level.hitbox.width + (17 * s) + 4 * s;
    C.btn_wiki.hitbox = (Rectangle){x7, y0, 4 * bw, bh};
    x7 += C.btn_wiki.hitbox.width + 4 * s;
    C.btn_sound.hitbox = (Rectangle){x7, y0, 6 * bw, bh};
    x7 += C.btn_sound.hitbox.width + 4 * s;
    C.btn_sound_paint.hitbox = (Rectangle){x7, y0, 6 * bw, bh};
    x7 += C.btn_sound_paint.hitbox.width + 4 * s;
    C.btn_always_on_top.hitbox = (Rectangle){x7, y0, 6 * bw, bh};
    x7 += C.btn_always_on_top.hitbox.width + 4 * s;
    C.btn_neon.hitbox = (Rectangle){x7, y0, 4 * bw, bh};
    x7 += C.btn_neon.hitbox.width + 4 * s;
  }

  int sh = GetScreenHeight() / s;
  {
    int bx0 = 4 * s;
    int bx1 = bx0 + 18 * s;
    int by0 = (30) * s;
    // C.btn_level.hitbox =
    //     (Rectangle){bx0, by0, (2 * 17 + 1) * s, (2 * 17 + 1) * s};
    // by0 += C.btn_level.hitbox.height + 4 * s;

    C.btn_simu.hitbox = (Rectangle){bx0, by0, (17) * s, 17 * s};
    C.btn_pause.hitbox = (Rectangle){bx1, by0, (17) * s, 17 * s};
    by0 = by0 + 18 * s;
    C.btn_rewind.hitbox = (Rectangle){bx0, by0, (17) * s, 17 * s};
    C.btn_forward.hitbox = (Rectangle){bx1, by0, (17) * s, 17 * s};
    by0 = by0 + 18 * s;
    by0 = by0 + 4 * s;
    int by1 = by0 + 18 * s;
    int by2 = by1 + 18 * s;
    int by3 = by2 + 18 * s + 4 * s;
    int by4 = by3 + 18 * s;
    int by5 = by4 + 18 * s;
    int by5b = by4 + 18 * s + 4 * s;

    int bw = 17 * s;
    int bh = 17 * s;

    C.btn_brush.hitbox = (Rectangle){bx0, by0, bw, bh};
    C.btn_line.hitbox = (Rectangle){bx1, by0, bw, bh};
    C.btn_marquee.hitbox = (Rectangle){bx0, by1, bw, bh};
    C.btn_text.hitbox = (Rectangle){bx1, by1, bw, bh};
    C.btn_bucket.hitbox = (Rectangle){bx0, by2, bw, bh};
    C.btn_picker.hitbox = (Rectangle){bx1, by2, bw, bh};

    C.btn_line_sep.hitbox = (Rectangle){bx0, by3, bw, bh};
    C.btn_line_sep_r.hitbox = (Rectangle){bx1, by3, bw, bh};

    C.btn_fliph.hitbox = (Rectangle){bx0, by3, bw, bh};
    C.btn_flipv.hitbox = (Rectangle){bx1, by3, bw, bh};
    C.btn_rotate.hitbox = (Rectangle){bx0, by4, bw, bh};
    C.btn_fill.hitbox = (Rectangle){bx1, by4, bw, bh};
    C.btn_stamp.hitbox = (Rectangle){bx0, by5b, bw, bh};
    C.btn_blueprint_add.hitbox = (Rectangle){bx1, by5b, bw, bh};
    C.btn_sel_open.hitbox = (Rectangle){bx0, by5b + 18 * s + 4 * s, bw, bh};
    C.btn_sel_save.hitbox = (Rectangle){bx1, by5b + 18 * s + 4 * s, bw, bh};

    C.btn_clockopt[0].hitbox = (Rectangle){bx0, by3, bw, bh};
    C.btn_clockopt[1].hitbox = (Rectangle){bx1, by3, bw, bh};
    C.btn_clockopt[2].hitbox = (Rectangle){bx0, by4, bw, bh};
    C.btn_clockopt[3].hitbox = (Rectangle){bx1, by4, bw, bh};
    C.btn_clockopt[4].hitbox = (Rectangle){bx0, by5, bw, bh};
    C.btn_clockopt[5].hitbox = (Rectangle){bx1, by5, bw, bh};
    int yy = by5 + C.btn_clockopt[5].hitbox.height;
    yy += 4 * s;
    C.btn_sim_show_t.hitbox = (Rectangle){bx0, yy, bw, bh};

    int cy = (sh - 2 * 18 - 2) * s;
    int cx = 4 * s + 35 * s + 4 * s;
    C.fg_color_rect = (Rectangle){
        .x = cx,
        .y = cy,
        .width = 2 * 17 * s,
        .height = 2 * 17 * s,
    };
    for (int i = 0; i < 64; i++) {
      int bx = i / 2;
      int by = i % 2;
      C.color_btn[i] = (Rectangle){
          .x = cx + (2 + bx) * 17 * s,
          .y = cy + (by) * (17 * s),
          .width = 1 * 17 * s,
          .height = 1 * 17 * s,
      };
    }

    {
      int x = C.fg_color_rect.x;
      int y = C.fg_color_rect.y;
      int h = C.fg_color_rect.height;
      int w = C.color_btn[17].x + C.color_btn[17].width - x;
      C.energy_rect = (Rectangle){
          .x = x,
          .y = y,
          .width = w,
          .height = h,
      };
    }

    /* bottom of right column */

    {
      int xx = cx + s * 17 * 13;
      int pad = 4 * s;
      int bw = 1 * 17 * s;
      // int xoff = sw - pad - bw;
      int xoff = pad;  // sw - pad - bw;

      C.btn_layer_push.hitbox = (Rectangle){
          xoff,
          cy + 10 * s,
          17 * s,
          10 * s,
      };
      C.btn_layer_pop.hitbox = (Rectangle){
          xoff,
          cy + 10 * s + 10 * s + s,
          17 * s,
          10 * s,
      };
      for (int i = 0; i < MAX_LAYERS; i++) {
        C.btn_layer[i].hitbox = (Rectangle){
            .x = xoff,
            .y = cy - bw * (i + 1),
            .width = bw,
            .height = bw,
        };
        C.btn_layer_v[i].hitbox = (Rectangle){
            .x = xoff + bw + 2,
            .y = cy - bw * (i + 1),
            .width = bw,
            .height = bw,
        };
      }
    }
  }
}

void main_update_viewport() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  main_update_layout();
  int pad = 4;
  int scale = 2;
  int tt = 2 * scale;
  C.target_pos = (Vector2){
      .x = pad + (42) * scale + tt + 4,
      .y = C.header_size + 2 * scale + tt,
  };
  int tgt_size_x = sw - C.target_pos.x - 4 -
                   4 * scale;  // - pad - tt - 8 * scale - 35 * scale;

  int tgt_size_y = sh - C.bottom_size - C.header_size - tt - 4;

  // Avoids crashing when window is too small
  const int min_tgt_size = 32;
  tgt_size_x = maxint(min_tgt_size, tgt_size_x);
  tgt_size_y = maxint(min_tgt_size, tgt_size_y);
  if (tgt_size_x != C.img_target_tex.texture.width ||
      tgt_size_y != C.img_target_tex.texture.height) {
    if (C.img_target_tex.texture.width > 0) {
      UnloadRenderTexture(C.img_target_tex);
      UnloadRenderTexture(C.level_overlay_tex);
    }
    C.img_target_tex = gen_render_texture(tgt_size_x, tgt_size_y, BLANK);
    C.level_overlay_tex = gen_render_texture(tgt_size_x, tgt_size_y, BLANK);
  }
}

static char* main_get_filename() {
  if (C.fname) {
    return C.fname;
  }
  return "Untitled";
}

void main_update_title() {
  const char* fname = GetFileName(main_get_filename());
  char tmp[400];
  if (strlen(fname) >= 400) {
    snprintf(tmp, sizeof(tmp), "%.*s ... - Circuit Artist", 400, fname);
  } else {
    snprintf(tmp, sizeof(tmp), "%s - Circuit Artist", fname);
  }
  SetWindowTitle(tmp);
}

void main_draw_error_message(const char* msg) {
  int x = C.target_pos.x;
  int y = C.target_pos.y;
  rlPushMatrix();
  rlTranslatef(x + 20, y + 20, 0);
  int scale = ui_get_scale();
  rlScalef(scale, scale, 1);
  int tw = get_rendered_text_size(msg).x;
  int lh = get_font_line_height();
  int th = 17;
  int dy = (th - lh) / 2;
  int pad = 10;
  DrawRectangle(0, 0, tw + 2 * pad, th + 2 * pad, RED);
  draw_text_box(msg, (Rectangle){pad, pad + dy, 1000, 0}, WHITE, NULL);
  rlPopMatrix();
}

void main_draw_status_bar() {
  int ui_scale = ui_get_scale();
  Color tc = WHITE;
  int num_lines = 7;
  int yc1 = (C.target_pos.y + C.img_target_tex.texture.height) / ui_scale -
            19 * num_lines;
  int xc = 1 * 4 + C.target_pos.x / ui_scale;
  int yc2 = yc1 + 17;
  int yc3 = yc2 + 17;
  int yc4 = yc3 + 17;
  int yc5 = yc4 + 17;
  int yc6 = yc5 + 17;
  int yc7 = yc6 + 17;
  int yc8 = yc7 + 17;
  int yc9 = yc8 + 17;
  rlPushMatrix();
  Color c2 = get_lut_color(COLOR_BTN2);

  tc = c2;
  rlScalef(ui_scale, ui_scale, 1);
  char txt[500];
  Color bg = BLACK;
  // Status bar stuff.
  Paint* pnt = &C.ca;
  if (pnt->mouseOnTarget) {
    v2i cursor = paint_get_cursor(pnt);
    snprintf(txt, sizeof(txt), "X: %d Y: %d", cursor.x, cursor.y);
    font_draw_texture_outlined(txt, xc, yc1, tc, bg);
    if (main_get_simu_mode() == MODE_EDIT) {
      v2i selSize = paint_get_active_sel_size(pnt);
      int sw = selSize.x;
      int sh = selSize.y;
      if (sw >= 0) {
        snprintf(txt, sizeof(txt), "w: %d h: %d (%d pixels)", sw, sh, sw * sh);
        font_draw_texture_outlined(txt, xc, yc2, tc, bg);
      }
    }
    int zoomPerc = paint_get_zoom_perc(pnt);
    snprintf(txt, sizeof(txt), "Z: %d%%", zoomPerc);
    font_draw_texture_outlined(txt, xc, yc3, tc, bg);
  }
  if (main_get_simu_mode() == MODE_SIMU) {
    // int energy = C.sim.state.total_energy;
    int energy = C.sim.state.total_energy;
    snprintf(txt, sizeof(txt), "E: %.1lf", C.sim.state.total_energy);
    font_draw_texture_outlined(txt, xc, yc5, tc, bg);

    snprintf(txt, sizeof(txt), "T: %d", C.sim.state.cur_tick);
    font_draw_texture_outlined(txt, xc, yc6, tc, bg);
  }
  const char* fname = GetFileName(main_get_filename());
  snprintf(txt, sizeof(txt), "[img] %s", fname);
  font_draw_texture_outlined(txt, xc, yc7, tc, bg);
  v2i buf_size = hist_get_buf_size(&C.ca.h);
  snprintf(txt, sizeof(txt), "[img] w: %d h: %d", buf_size.x, buf_size.y);
  font_draw_texture_outlined(txt, xc, yc8, tc, bg);
  rlPopMatrix();
}

RectangleInt main_get_target_region() {
  return (RectangleInt){
      .x = C.target_pos.x,
      .y = C.target_pos.y,
      .width = C.img_target_tex.texture.width,
      .height = C.img_target_tex.texture.height,
  };
}

void main_draw_mouse_extra() {
  if (main_get_simu_mode() == MODE_EDIT && paint_get_tool(&C.ca) == TOOL_LINE &&
      C.mouseIsPen) {
    Vector2 pos = GetMousePosition();
    int s = ui_get_scale();
    bool just_changed = paint_get_key_line_width_has_just_changed(&C.ca);
    char txt[50];
    snprintf(txt, sizeof(txt), "w=%d", paint_get_line_width(&C.ca));
    rlPushMatrix();
    rlTranslatef(pos.x + 8 * s, pos.y + 8 * s, 0);
    rlScalef(s, s, 1);
    font_draw_texture(txt, 0, 0, (just_changed ? YELLOW : WHITE));
    int sep = paint_get_line_sep(&C.ca);
    if (sep != 1) {
      rlTranslatef(0, 10, 0);
      snprintf(txt, sizeof(txt), "s=%d", sep);
      font_draw_texture(txt, 0, 0, WHITE);
    }
    rlPopMatrix();
  }
  if (strlen(C.mouse_msg) > 0) {
    Vector2 pos = GetMousePosition();
    int s = ui_get_scale();
    rlPushMatrix();
    rlTranslatef(pos.x + 8 * s, pos.y + 8 * s, 0);
    rlScalef(s, s, 1);
    Color msg_color = BLANK;
    if (C.mouse_msg_type == 0) msg_color = WHITE;
    if (C.mouse_msg_type == 1) msg_color = RED;
    font_draw_texture_outlined(C.mouse_msg, 0, 0, msg_color, BLACK);
    rlPopMatrix();
  }
}

void main_check_file_drop() {
  if (!IsFileDropped()) {
    return;
  }
  FilePathList path_list = LoadDroppedFiles();
  if (path_list.count > 0) {
    const char* fname = path_list.paths[0];
    Image img = LoadImage(fname);
    if (img.width > 0) {
      paint_paste_image(&C.ca, img, 0);
    }
  }
  UnloadDroppedFiles(path_list);
}

void main_update_widgets() {
  bool ned = C.mode != MODE_EDIT;
  int tool = paint_get_tool(&C.ca);
  bool demo = ui_is_demo();

  C.btn_new.disabled = ned;
  C.btn_open.disabled = ned || demo;

  C.btn_brush.disabled = ned;
  C.btn_line.disabled = ned;
  C.btn_marquee.disabled = ned;
  C.btn_text.disabled = ned;
  C.btn_bucket.disabled = ned;
  C.btn_picker.disabled = ned;

  C.btn_sound.toggled = !C.mute;
  C.btn_neon.toggled = C.use_neon;
  C.btn_sound_paint.toggled = !C.muted_paint;
  C.btn_always_on_top.toggled = C.always_on_top;
  C.btn_pause.toggled = C.paused;
  C.btn_rewind.toggled = C.rewind_pressed;
  C.btn_forward.toggled = C.forward_pressed;

  C.btn_rewind.disabled = !ned;
  C.btn_forward.disabled = !ned || !C.paused;
  C.btn_pause.disabled = !ned;

  C.btn_level.disabled = ned;
  C.btn_stamp.disabled = ned;
  C.btn_brush.toggled = tool == TOOL_BRUSH;
  C.btn_line.toggled = tool == TOOL_LINE;
  C.btn_marquee.toggled = tool == TOOL_SEL;
  C.btn_bucket.toggled = tool == TOOL_BUCKET;
  C.btn_picker.toggled = tool == TOOL_PICKER;

  int has_sel = paint_get_has_selection(&C.ca) && tool == TOOL_SEL;
  C.btn_rotate.disabled = !has_sel || ned;
  C.btn_flipv.disabled = !has_sel || ned;
  C.btn_fliph.disabled = !has_sel || ned;
  C.btn_fill.disabled = !has_sel || ned;
  C.btn_line_sep.disabled = ned;
  C.btn_line_sep_r.disabled = ned || paint_get_line_sep(&C.ca) == 1;
  C.btn_sel_open.disabled = ned;
  C.btn_sel_save.disabled = !has_sel || ned;
  C.btn_blueprint_add.disabled = !has_sel || ned;
  C.btn_rotate.hidden = (tool != TOOL_SEL) || ned;
  C.btn_flipv.hidden = (tool != TOOL_SEL) || ned;
  C.btn_fliph.hidden = (tool != TOOL_SEL) || ned;
  C.btn_fill.hidden = (tool != TOOL_SEL) || ned;
  C.btn_line_sep.hidden = (tool != TOOL_LINE) || ned;
  C.btn_line_sep_r.hidden = (tool != TOOL_LINE) || ned;
  C.btn_sel_open.hidden = (tool != TOOL_SEL) || ned;
  C.btn_sel_save.hidden = (tool != TOOL_SEL) || ned;
  C.btn_blueprint_add.hidden = (tool != TOOL_SEL) || ned;
  C.btn_stamp.hidden = (tool != TOOL_SEL) || ned;

  for (int i = 0; i < MAX_LAYERS; i++) {
    C.btn_layer[i].disabled = ned;
    // C.btn_layer_v[i].disabled = ned;
  }

  for (int i = 0; i < 6; i++) {
    C.btn_clockopt[i].hidden = C.mode != MODE_SIMU;
    C.btn_clockopt[i].toggled = C.clock_speed == i;
  }
  C.btn_sim_show_t.toggled = C.sim_show_t;
  C.btn_sim_show_t.hidden = C.mode != MODE_SIMU;

  int nl = paint_get_num_layers(&C.ca);
  C.btn_layer_pop.disabled = ned || (nl == 1);
  C.btn_layer_push.disabled =
      ned || (paint_get_num_layers(&C.ca) == MAX_LAYERS);  // MAX_LAYERS;
  for (int l = 0; l < MAX_LAYERS; l++) {
    C.btn_layer[l].hidden = l >= nl;
    C.btn_layer_v[l].hidden = l >= nl;
    C.btn_layer[l].toggled = hist_get_active_layer(&C.ca.h) == l;
    C.btn_layer_v[l].toggled = paint_get_layer_vis(&C.ca, l);
  }

  {
    RectangleInt r = main_get_target_region();
    paint_set_viewport(&C.ca, r);
  }
}

void main_new_file() {
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  paint_new_buffer(&C.ca);
  main_update_title();
}

void main_destroy() {
  paint_destroy(&C.ca);
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  UnloadRenderTexture(C.img_target_tex);
}

void main_load_image_from_path(const char* path) {
  paint_load_image(&C.ca, LoadImage(path));
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  C.fname = clone_string(path);
  main_update_title();
}

void main_open_file_modal() {
  on_modal_before_open();
  ModalResult mr = modal_open_file(NULL);
  on_modal_after_open();
  if (mr.ok) {
    main_load_image_from_path(mr.fPath);
    free(mr.fPath);
  } else if (mr.cancel) {
  } else {
    char txt[500];
    snprintf(txt, sizeof(txt), "ERROR: %s\n", mr.errMsg);
    msg_add(txt, MSG_DURATION);
  }
}

int main_on_save_click(bool saveas) {
  if (C.fname == NULL || saveas) {
    on_modal_before_open();
    ModalResult mr = modal_save_file(NULL, NULL);
    on_modal_after_open();
    if (mr.ok) {
      if (C.fname) {
        free(C.fname);
        C.fname = NULL;
      }
      C.fname = mr.fPath;
    } else if (mr.cancel) {
      return 1;
    } else {
      char txt[500];
      snprintf(txt, sizeof(txt), "ERROR: %s\n", mr.errMsg);
      msg_add(txt, MSG_DURATION);
      return -1;
    }
  }

  if (C.fname) {
    Image out = paint_export_buf(&C.ca);
    paint_set_not_dirty(&C.ca);
    if (!ExportImage(out, C.fname)) {
      msg_add("ERROR: Could not save image...", MSG_DURATION);
      return -2;
    }
    UnloadImage(out);
    main_update_title();
    msg_add("Image Saved.", MSG_DURATION);
    return 0;
  }
  return 0;
}

static void on_dialog_close(int r) {
  if (r == 2 || r == -1) {  // cancel: no action
    return;
  }
  if (r == 0) {  // save
    if (main_on_save_click(false)) {
      return;  // Cancelled during save
    }
  }
  // Exectued on resul=0 or result=1
  C.dialog_callback();
}

void main_ask_for_save_and_proceed(Callback next_action) {
  C.dialog_callback = next_action;
  if (!paint_get_is_dirty(&C.ca)) {
    next_action();
  } else {
    dialog_open("Do you want to save changes?", "Save", "Don't save", "Cancel",
                on_dialog_close);
  }
}

void main_paste_text(const char* txt) {
  if (strlen(txt) == 0) return;
  Image img = render_text(txt, C.ca.fg_color);
  paint_paste_image(&C.ca, img, 0);
}

void main_set_line_sep(int n) {
  if (n <= 1) n = 1;
  if (n >= 128) n = 128;
  paint_set_line_sep(&C.ca, n);
}

SimuState main_get_simu_state() {
  return (SimuState){
      .done = C.sim.state.done,
      .error = C.sim.state.error,
  };
}

void main_paste_file(const char* fname, int rot) {
  Image img = LoadImage(fname);
  if (img.width == 0) {
    msg_add(TextFormat("ERROR: Could not open image %s", fname), 10);
    return;
  }
  paint_paste_image(&C.ca, img, rot);
  // msg_add(TextFormat("Pasted %s", fname), 3);
}

Paint* main_get_paint() { return &C.ca; }

void play_sound(SoundEnum sound) {
  switch (sound) {
    case SOUND_LEVEL_COMPLETE:
      PlaySound(C.sound_success);
      break;
  }
}
