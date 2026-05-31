#include "win_main.h"

#include <msgpack.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "blueprint.h"
#include "colors.h"
#include "common.h"
#include "discord_integration.h"
#include "font.h"
#include "i18n.h"
#include "img.h"
#include "layout.h"
#include "log.h"
#include "lua_level.h"
#include "math.h"
#include "modal.h"
#include "msg.h"
#include "paint.h"
#include "paths.h"
#include "profiler.h"
#include "raylib.h"
#include "sim.h"
#include "sound.h"
#include "stb_ds.h"
#include "time.h"
#include "ui.h"
#include "uifont.h"
#include "utils.h"
#include "wabout.h"
#include "wdialog.h"
#include "widgets.h"
#include "win_blueprint.h"
#include "win_customlvl.h"
#include "win_level.h"
#include "win_msg.h"
#include "win_settings.h"
#include "win_wiki.h"
#include "wnumber.h"
#include "wtext.h"

static struct {
  Layout* layout;
  int mode;              // Edit, Simu
  RTex2D img_target_tex; /* Image Drawing target image */
  RTex2D
      level_overlay_tex; /* Image overlay where the level stuff can draw to. */
  v2 target_pos;
  GameRegistry* r;
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
  Btn btn_exit;

  Btn btn_wiki;
  Btn btn_blueprint;
  Btn btn_blueprint_add;

  Btn btn_pause;
  Btn btn_rewind;
  Btn btn_forward;

  // Tools buttons
  Btn btn_level_custom;
  Btn btn_level_campaign;
  Btn btn_simu;
  Btn btn_brush;
  Btn btn_text;
  Btn btn_line;
  Btn btn_picker;
  Btn btn_bucket;
  Btn btn_marquee;
  Btn btn_rotate;
  Btn btn_fill;
  Btn btn_fliph;
  Btn btn_settings;
  Btn btn_flipv;
  Btn btn_sel_open;
  Btn btn_sel_save;
  Btn btn_line_sep;
  Btn btn_line_sep_r;

  Btn btn_layer_push;
  Btn btn_layer_pop;
  Btn btn_layer[MAX_LAYERS];
  Btn btn_layer_v[MAX_LAYERS];

  // speed buttons
  Btn btn_clockopt[6];
  Btn btn_sim_show_t;
  Btn btn_sim_soladd;

  Sim sim;
  HSim hsim;
  bool rewind_pressed;
  bool forward_pressed;
  bool paused;
  double simu_target_steps;
  int pix_toggle;
  bool mouseIsPen;
  Image sidepanel_img;
  Texture2D sidepanel_tex;
  bool looping;

  int clock_speed;

  bool time_open;
  double time_ref;
  char mouse_msg[200];
  int mouse_msg_type;
  bool sim_show_t;

  /* If true, it means there was an error in the kernel/lua
   * side, and the simulation can't start.   */
  bool kernel_error;
  char* kernel_error_msg;

  double hover_wire_distance;
  v2 time_c;
  v2 time_pos_ref;
  // lua_State* L; /* Lua context used for configuration */
  Rectangle energy_rect;
  void (*dialog_callback)();
  LevelDef* ldef;
  CustomLevelDef* cldef;
  LevelAPI api;
  Blueprint* bp;
} C = {0};

static inline int maxint(int a, int b) { return a > b ? a : b; }

#define MSG_DURATION 2

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

static int s_last_num_nands = 0;

static const char* get_filename() {
  if (C.fname) {
    return C.fname;
  }
  return T.main_untitled;
}

static void update_title() {
  const char* fname = GetFileName(get_filename());
  char tmp[400];
  if (strlen(fname) >= 400) {
    snprintf(tmp, sizeof(tmp), "%.*s ... - %s", 400, fname, T.ca);
  } else {
    snprintf(tmp, sizeof(tmp), "%s - %s", fname, T.ca);
  }
  SetWindowTitle(tmp);
}

void update_layout() {
  Layout* l = C.layout;
  /* No layout_update_offset: full-screen layout, offset stays {0,0} */
  int svg_h = (int)layout_rect(l, "window").height;
  int svg_w = (int)layout_rect(l, "window").width;
  int dy = GetScreenHeight() - svg_h;
  int dx = GetScreenWidth() - svg_w;

  /* Top-bar buttons */
  C.btn_new.hitbox = layout_rectb(l, "btn_new");
  C.btn_open.hitbox = layout_rectb(l, "btn_open");
  C.btn_save.hitbox = layout_rectb(l, "btn_save");
  C.btn_saveas.hitbox = layout_rectb(l, "btn_saveas");
  C.btn_settings.hitbox = layout_rectb(l, "btn_settings");
  C.btn_exit.hitbox = layout_rectb(l, "btn_exit");
  C.btn_level_campaign.hitbox = layout_rectb(l, "btn_campaign_level");
  C.btn_level_custom.hitbox = layout_rectb(l, "btn_custom_level");
  C.btn_blueprint.hitbox = layout_rectb(l, "btn_blueprint");
  C.btn_sim_soladd.hitbox = layout_rectb(l, "btn_soladd");
  C.btn_blueprint_add.hitbox = layout_rectb(l, "btn_bluadd");
  C.btn_wiki.hitbox = layout_rectb(l, "btn_wiki");

  /* Left-bar play/simu controls */
  C.btn_simu.hitbox = layout_rectb(l, "btn_play");
  C.btn_pause.hitbox = layout_rectb(l, "btn_pause");
  C.btn_rewind.hitbox = layout_rectb(l, "btn_rewind");
  C.btn_forward.hitbox = layout_rectb(l, "btn_forward");

  /* Left-bar tool buttons */
  C.btn_brush.hitbox = layout_rectb(l, "btn_brush");
  C.btn_line.hitbox = layout_rectb(l, "btn_line");
  C.btn_marquee.hitbox = layout_rectb(l, "btn_marquee");
  C.btn_text.hitbox = layout_rectb(l, "btn_text");
  C.btn_bucket.hitbox = layout_rectb(l, "btn_bucket");
  C.btn_picker.hitbox = layout_rectb(l, "btn_picker");

  /* Extra slots: shared between edit-mode tools and simu-mode clock options */
  Rectangle extra[8];
  for (int i = 0; i < 8; i++) {
    extra[i] = layout_rectb(l, TextFormat("btn_extra%d", i + 1));
  }
  C.btn_line_sep.hitbox = extra[0];
  C.btn_line_sep_r.hitbox = extra[1];
  C.btn_fliph.hitbox = extra[0];
  C.btn_flipv.hitbox = extra[1];
  C.btn_rotate.hitbox = extra[2];
  C.btn_fill.hitbox = extra[3];
  C.btn_sel_open.hitbox = extra[4];
  C.btn_sel_save.hitbox = extra[5];
  for (int i = 0; i < 6; i++) C.btn_clockopt[i].hitbox = extra[i];
  C.btn_sim_show_t.hitbox = extra[6];

  /* Bottom-aligned: shift y by (actual screen height - SVG design height) */
  C.fg_color_rect = layout_rect(l, "rect_fg_color");
  C.fg_color_rect.y += dy;

  for (int i = 0; i < 32; i++) {
    C.color_btn[i] = layout_rect(l, TextFormat("c%d", i + 1));
    C.color_btn[i].y += dy;
  }

  for (int i = 0; i < MAX_LAYERS; i++) {
    C.btn_layer[i].hitbox = layout_rectb(l, TextFormat("btn_layer%d", i + 1));
    C.btn_layer[i].hitbox.y += dy;
    C.btn_layer_v[i].hitbox =
        layout_rectb(l, TextFormat("btn_layer%d_hide", i + 1));
    C.btn_layer_v[i].hitbox.y += dy;
  }

  C.btn_layer_push.hitbox = layout_rectb(l, "btn_layer_add");
  C.btn_layer_push.hitbox.y += dy;
  C.btn_layer_pop.hitbox = layout_rectb(l, "btn_layer_rm");
  C.btn_layer_pop.hitbox.y += dy;
}

static void discord_refresh() {
  const char* fname = GetFileName(get_filename());
  const char* level_name = NULL;
  if (C.ldef)
    level_name = C.ldef->name;
  else if (C.cldef)
    level_name = C.cldef->name;

  if (C.mode == MODE_EDIT) {
    discord_set_editing(fname, level_name, s_last_num_nands);
  } else {
    discord_set_simulating(fname, level_name, s_last_num_nands);
  }
}

static bool main_can_rewind() { return C.api.bw != NULL; }

static void on_modal_before_open() {
  if (is_always_on_top()) {
    ClearWindowState(FLAG_WINDOW_TOPMOST);
  }
}

static void on_modal_after_open() {
  if (is_always_on_top()) {
    SetWindowState(FLAG_WINDOW_TOPMOST);
  }
}

static void unlink_bp() {
  if (C.bp) {
    win_msg_open_text(T.main_blueprint_unlinked, NULL);
  }
  C.bp = NULL;
}

/* called after an image has been saved */
static void on_post_image_save(bool saveas) {
  if (saveas) unlink_bp();
  if (C.bp) {
    Image imgs[MAX_LAYERS];
    int nl = hist_get_num_layers(&C.ca.h);
    for (int i = 0; i < nl; i++) {
      imgs[i] = C.ca.h.buffer[i];
    }
    blueprint_update_thumbnail(C.bp, nl, imgs);
    // Now does the linking.
    const char* id = NULL;
    if (C.ldef) {
      id = C.ldef->id;
    } else if (C.cldef) {
      id = C.cldef->id;
    }
    bool solved = false;
    if (C.mode == MODE_SIMU && C.sim.complete) {
      solved = true;
    }
    bool was_solved = C.bp->solved_level;
    bool had_link = C.bp->linked_level_id != NULL;
    blueprint_link_to_level(C.bp, id, solved);
    if (id && !had_link) {
      msg_add(TextFormat(T.main_blueprint_linked, get_level_name_by_id(id)),
              MSG_DURATION);
    } else if (id && !was_solved && solved) {
      msg_add(TextFormat(T.main_blueprint_marked, get_level_name_by_id(id)),
              MSG_DURATION);
    }
    msg_add(T.main_blueprint_saved, MSG_DURATION);
  } else {
    msg_add(T.main_image_saved, MSG_DURATION);
  }
  update_title();
}

void win_main_open() {
  ui_winpush(WINDOW_MAIN);
  easy_blinking_open();
}

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

/*
 * dt = ticks per second.
 * */
static float get_simu_dt() { return get_speed_dt(C.clock_speed); }

static inline int get_simu_speed() {
  if (C.rewind_pressed) return -1;
  if (C.forward_pressed) return 1;
  return (C.paused || C.time_open) ? 0 : 1;
}

static float get_simu_slack_steps() {
  return C.simu_target_steps - C.sim.state.cur_tick;
}

void load_palette_asset(const char* asset) {
  Image pal = load_image_asset(asset);
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

static void show_kernel_error() {
  about_open("Script Error", C.kernel_error_msg, NULL);
}

void win_main_stop_simu() {
  // assert(main_get_simu_mode() != MODE_EDIT);
  assert(main_get_simu_mode() == MODE_SIMU ||
         main_get_simu_mode() == MODE_ERROR);
  hsim_destroy(&C.hsim);
  sim_destroy(&C.sim);

  C.paused = false;
  C.mode = MODE_EDIT;
  C.time_open = false;
  C.rewind_pressed = false;
  C.forward_pressed = false;
  discord_refresh();
}

static void handle_kernel_error(Status s) {
  assert(!C.kernel_error);
  if (main_is_simulation_on()) {
    win_main_stop_simu();
  }
  C.kernel_error = true;
  msg_add("Level error :(", 5);
  printf("------------- LEVEL ERROR -------------\n");
  printf("%s\n", s.err_msg);
  printf("-------------------------------------\n");
  C.kernel_error_msg = s.err_msg;
  show_kernel_error();
}
static void reset_kernel_error() {
  if (C.kernel_error_msg) free(C.kernel_error_msg);
  C.kernel_error_msg = NULL;
  C.kernel_error = false;
}

static void reset_level() {
  C.ldef = NULL;
  C.cldef = NULL;
  level_api_destroy(&C.api);
  main_new_file();
  reset_kernel_error();
}

void win_main_load_custom_level(CustomLevelDef* ldef) {
  reset_level();
  C.cldef = ldef;
  Status s = lua_level_create_custom(&C.api, ldef);
  if (!s.ok) {
    handle_kernel_error(s);
  } else {
    msg_add(T.main_custom_level_loaded, 2);
  }
  discord_refresh();
}

static void load_campaign_level(LevelDef* ldef) {
  reset_level();
  C.ldef = ldef;
  Status s = lua_level_create_campaign(&C.api, ldef);
  if (!s.ok) {
    handle_kernel_error(s);
  } else {
    msg_add(T.main_campaign_loaded, 2);
  }
  discord_refresh();
}

void main_load_by_level_id(const char* id) {
  if (!id) return;
  if (starts_with(id, "campaign:")) {
    LevelDef* ldef = get_level_by_id(id);
    if (ldef) {
      load_campaign_level(ldef);
    }
  } else {
    CustomLevelDef* ldef = find_custom_level_by_id(id);
    if (ldef) {
      win_main_load_custom_level(ldef);
    }
  }
}

static void reload_level() {
  if (C.cldef) win_main_load_custom_level(C.cldef);
  if (C.ldef) load_campaign_level(C.ldef);
}

static void on_select_level(LevelDef* ldef) { load_campaign_level(ldef); }

static void update_viewport() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  update_layout();

  Layout* l = C.layout;
  Rectangle canvas = layout_rect(l, "canvas");
  Rectangle window = layout_rect(l, "window");

  int right_margin = (int)(window.width - (canvas.x + canvas.width));
  int bottom_margin = (int)(window.height - (canvas.y + canvas.height));
  C.target_pos = (Vector2){.x = canvas.x, .y = canvas.y};
  int tgt_size_x = sw - (int)canvas.x - right_margin;
  int tgt_size_y = sh - (int)canvas.y - bottom_margin;

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

static void load_initial_image() {
  if (false) {
    Image img = LoadImage("../a.png");
    paint_load_image(&C.ca, img);
  } else {
    if (ui_is_demo()) {
      Image img = load_image_asset("circuits/help_small.png");
      paint_load_image(&C.ca, img);
    } else {
      Image img = load_image_asset("imgs/tutorial.png");
      paint_load_image(&C.ca, img);
    }
    // paint_new_buffer(&C.ca);
  }
}

void win_main_init() {
  srand(time(NULL));
  C.kernel_error = false;
  C.layout = easy_load_layout("main");
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

  C.num_colors = 20;
  load_palette_asset("imgs/pal.png");
  int s = ui_get_scale();
  C.header_size = 24 * s;
  C.bottom_size = 3 * 17 * 1 * s - 6 * s;
  paint_init(&C.ca);
  paint_set_color(&C.ca, C.palette[3]);
  C.sidepanel_img = gen_image_simple(200, 800);
  C.sidepanel_tex = LoadTextureFromImage(C.sidepanel_img);
  update_viewport();
  update_title();
  main_update_widgets();
  C.r = getreg();
  sim_dry_run();
  win_main_load_custom_level(find_sandbox_custom_level());
  discord_init();
  discord_refresh();
  load_initial_image();
}

static void simu_play_sounds() {
  int na = arrlen(C.sim.ui_events);
  int r = 0;
  for (int i = 0; i < na; i++) {
    SimUiEvent ev = C.sim.ui_events[i];
    if (ev.sound > 0) {
      play_sound_nand();
    }
  }
}

/*
 * Runs simulation to ensure slack step is always 0 <= slack_step < 1
 */
static Status main_update_simu() {
  Status status = status_ok();
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

  profiler_tac();
  // assert(get_simu_slack_steps() >= 0 && get_simu_slack_steps() < 1.f);
  return status;
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

static LevelAPI* getlevel() { return &C.api; }

Status main_draw_level_kernel() {
  if (!main_is_simulation_on()) return status_ok();
  LevelAPI* api = getlevel();
  if (!api->draw) return status_ok();
  BeginTextureMode(C.level_overlay_tex);
  rlPushMatrix();
  Status s = api->draw(api->u);
  rlPopMatrix();
  EndTextureMode();
  return s;
}

void win_main_update() {
  discord_run_callbacks();
  update_viewport();
  C.rewind_pressed = false;
  C.forward_pressed = false;
  main_check_file_drop();
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
    Status s = main_update_simu();
    if (!s.ok) {
      handle_kernel_error(s);
    }
  }
  profiler_tac();
  profiler_tic("Rendering");

  int mode = main_get_simu_mode();
  if (mode == MODE_EDIT) {
    Color k_normal = {21, 11, 3, 255};
    // Color k_blueprint = {3, 11, 31, 255};
    // C.ca.bg_color = (C.bp == NULL) ? k_normal : k_blueprint;
    C.ca.bg_color = k_normal;
    paint_render_texture(&C.ca, C.sidepanel_tex, C.img_target_tex);
    int w = paint_img_width(&C.ca);
    int h = paint_img_height(&C.ca);
    level_api_draw_pin_sockets(&C.api, C.ca.cam, w, h, C.img_target_tex);
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
                                  slack_steps, hide_mask, is_circuit_neon_on());
    profiler_tac();
    BeginTextureMode(C.img_target_tex);
    ClearBackground(PURPLE);
    EndTextureMode();
    texdraw2(C.img_target_tex, rendered->rt);
    texdel(rendered);
  }

  {
    int w = paint_img_width(&C.ca);
    int h = paint_img_height(&C.ca);
    level_api_draw_board(&C.api, C.ca.cam, w, h, C.level_overlay_tex);
  }

  if (!C.kernel_error) {
    Status s = status_ok();
    // if (s.ok) s = main_draw_level_board();
    if (s.ok) s = main_draw_level_kernel();
    if (!s.ok) handle_kernel_error(s);
  }

  profiler_tac();
  texcleanup();
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

void win_main_start_simu() {
  assert(!C.kernel_error);
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
  LevelAPI* api = getlevel();
  SimParams p = {
      .nl = nl,
      .img = &imgs[0],
      .api = api,
      .layers = &texs[0],
      .warmup_cycles = api->warmup_cycles,
  };
  Status s = sim_init(&C.sim, p);
  if (!s.ok) {
    handle_kernel_error(s);
    sim_destroy(&C.sim);
    return;
  }

  s_last_num_nands = arrlen(C.sim.pg.nands);
  C.hsim = wrap_sim(&C.sim);
  C.simu_target_steps = 0;
  C.pix_toggle = -1;
  if (sim_has_errors(&C.sim)) {
    play_sound_oops();
    C.mode = MODE_ERROR;
    discord_refresh();
    return;
  }
  C.mode = MODE_SIMU;
  discord_refresh();
}

static void main_toggle_simu() {
  int mode = C.mode;
  if (mode == MODE_EDIT) {
    win_main_start_simu();
  } else {
    win_main_stop_simu();
  }
}

static const char* get_level_id() {
  if (C.ldef) {
    return C.ldef->id;
  } else if (C.cldef) {
    return C.cldef->id;
  } else {
    return NULL;
  }
}

static void add_blueprint_as_solution() {
  if (!get_level_id()) return;
  win_main_stop_simu();
  Image full = paint_export_buf(&C.ca);
  int nl = hist_get_num_layers(&C.ca.h);
  const char* lvl_id = get_level_id();
  int ibp = blueprint_create_and_open(nl, C.ca.h.buffer, full, lvl_id);

  if (ibp >= 0) {
    Blueprint* bp = get_blueprint(&C.r->store, ibp);
    char* name = clone_string(
        TextFormat("Solution to %s", get_level_name_by_id(lvl_id)));
    blueprint_rename(bp, name);
    free(name);
  }
  if (ibp == -1) {
    msg_add(T.main_inventory_full, 3);
  }
  UnloadImage(full);
}

static void add_blueprint() {
  if (!paint_get_has_selection(&C.ca)) {
    return;
  }

  Image full = paint_export_sel(&C.ca);
  int nl = hist_get_num_layers_sel(&C.ca.h);
  int ibp = blueprint_create_and_open(nl, C.ca.h.selbuffer, full, NULL);
  if (ibp == -1) {
    msg_add(T.main_inventory_full, 3);
  }
  UnloadImage(full);
}

static void paste_text(const char* txt) {
  if (strlen(txt) == 0) return;
  Image img = uifont_render_text_1x(txt, C.ca.fg_color);
  paint_paste_image(&C.ca, img, 0);
}

static void on_paste_text(void* ctx, const char* txt) { paste_text(txt); }

static void on_open_click() {
  win_main_ask_for_save_and_proceed(main_open_file_modal);
}

static void on_new_click() { win_main_ask_for_save_and_proceed(main_new_file); }

static int on_save_click(bool saveas) {
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
      snprintf(txt, sizeof(txt), T.main_msg_error, mr.errMsg);
      msg_add(txt, MSG_DURATION);
      return -1;
    }
  }

  if (C.fname) {
    Image out = paint_export_buf(&C.ca);
    paint_set_not_dirty(&C.ca);
    if (!ExportImage(out, C.fname)) {
      UnloadImage(out);
      msg_add(T.main_could_not_save_image, MSG_DURATION);
      return -2;
    }
    UnloadImage(out);
    on_post_image_save(saveas);
    return 0;
  }
  return 0;
}

void main_update_controls() {
  if (is_control_down()) {
    if (IsKeyPressed(KEY_S)) {
      on_save_click(false);
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
    win_blueprint_open();
  }

  if (isEdit && IsKeyPressed(KEY_F1)) {
    play_sound_click();
    paint_set_layer(&C.ca, 0);
  }

  if (isEdit && IsKeyPressed(KEY_F2)) {
    if (paint_get_num_layers(&C.ca) > 1) {
      play_sound_click();
      paint_set_layer(&C.ca, 1);
    }
  }

  if (isEdit && IsKeyPressed(KEY_F3)) {
    if (paint_get_num_layers(&C.ca) > 2) {
      play_sound_click();
      paint_set_layer(&C.ca, 2);
    }
  }

  if (isEdit && IsKeyPressed(KEY_F5)) {
    reload_level();
  }

  // if (isEdit && IsKeyPressed(KEY_F6)) {
  //   win_log_open();
  // }

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
              strcpy(C.mouse_msg, T.main_wire_too_slow);
            }
            if (status == STATUS_CONFLICT) {
              C.mouse_msg_type = 1;
              strcpy(C.mouse_msg, T.main_more_than_one_nand);
            }
          }
        }
      }
    }
  }
  if (IsKeyPressed(KEY_SPACE) && !C.kernel_error) {
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
#if 0
  if (isEdit && IsKeyPressed(KEY_K)) {
    win_workshop_open(NULL);
  }

#endif

#if 0
  if (isEdit && IsKeyPressed(KEY_K)) {
    win_mtext_open(
        on_mtext_accept, NULL,
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed do eiusmod tempor incididunt ut labore et dolore magna "
        "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
        "ullamco laboris nisi ut aliquip ex ea commodo consequat.\n\n"
        "Duis aute irure dolor in reprehenderit in voluptate velit "
        "esse cillum dolore eu fugiat nulla pariatur.");
  }
#endif

  if (!isEdit) {
    if (IsKeyPressed(KEY_K)) {
      C.paused = !C.paused;
    }
    if (IsKeyDown(KEY_L)) C.forward_pressed = true;
    bool can_rewind = main_can_rewind();
    if (can_rewind) {
      if (IsKeyDown(KEY_J)) C.rewind_pressed = true;
    }
  }

  if (C.mode == MODE_SIMU) {
    bool can_rewind = main_can_rewind();
    if (can_rewind && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
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
    win_main_paste_file(mr.fPath, 0);
    free(mr.fPath);
  } else if (mr.cancel) {
  } else {
    char txt[500];
    snprintf(txt, sizeof(txt), T.main_msg_error, mr.errMsg);
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
    snprintf(txt, sizeof(txt), T.main_msg_error, mr.errMsg);
    msg_add(txt, MSG_DURATION);
    return;
  }

  if (mr.fPath && mr.ok) {
    Image out = paint_export_sel(&C.ca);
    // Before saving, add black pixels back
    if (!ExportImage(out, mr.fPath)) {
      msg_add(T.main_could_not_save_sel, MSG_DURATION);
      return;
    }
    UnloadImage(out);
    msg_add(T.main_sel_saved, MSG_DURATION);
    return;
  }
}

static void toggle_simu_pause() { C.paused = !C.paused; }

static void toggle_sim_show_t() { C.sim_show_t = !C.sim_show_t; }

static void custom_level_open_win() { win_customlvl_open(C.cldef); }

void win_main_open_level() { win_level_open(C.ldef, on_select_level); }

static void on_btn_campaign_level_click() {
  win_main_ask_for_save_and_proceed(win_main_open_level);
}

static void on_btn_custom_level_click() {
  win_main_ask_for_save_and_proceed(custom_level_open_win);
}

void main_update_hud() {
  Paint* ca = &C.ca;
  if (btn_update(&C.btn_new)) on_new_click();
  if (btn_update(&C.btn_open)) on_open_click();
  if (btn_update(&C.btn_settings)) win_settings_open();
  if (btn_update(&C.btn_save)) on_save_click(false);
  if (btn_update(&C.btn_saveas)) on_save_click(true);
  if (btn_update(&C.btn_exit)) ui_set_close_requested();

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
  if (btn_update(&C.btn_line_sep_r)) win_main_set_line_sep(1);

  if (btn_update(&C.btn_simu)) main_toggle_simu();
  if (btn_update(&C.btn_pause)) toggle_simu_pause();
  btn_update(&C.btn_rewind);
  btn_update(&C.btn_forward);
  if (C.btn_rewind.pressed) C.rewind_pressed = true;
  if (C.btn_forward.pressed) C.forward_pressed = true;

  if (btn_update(&C.btn_level_campaign)) on_btn_campaign_level_click();
  if (btn_update(&C.btn_level_custom)) on_btn_custom_level_click();
  if (btn_update(&C.btn_wiki)) win_wiki_open();
  if (btn_update(&C.btn_blueprint)) win_blueprint_open();

  if (btn_update(&C.btn_clockopt[0])) C.clock_speed = 0;
  if (btn_update(&C.btn_clockopt[1])) C.clock_speed = 1;
  if (btn_update(&C.btn_clockopt[2])) C.clock_speed = 2;
  if (btn_update(&C.btn_clockopt[3])) C.clock_speed = 3;
  if (btn_update(&C.btn_clockopt[4])) C.clock_speed = 4;
  if (btn_update(&C.btn_clockopt[5])) C.clock_speed = 5;
  if (btn_update(&C.btn_sim_show_t)) toggle_sim_show_t();
  if (btn_update(&C.btn_sim_soladd)) add_blueprint_as_solution();

  Vector2 pos = GetMousePosition();
  if (rect_hover(C.fg_color_rect, pos)) {
    ui_inc_hit_count();
  }
  for (int i = 0; i < C.num_colors; i++) {
    if (rect_hover(C.color_btn[i], pos) && ui_get_hit_count() == 0) {
      ui_inc_hit_count();
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        play_sound_click();
        paint_set_color(&C.ca, C.palette[i]);
      }
    }
  }
}

static void draw_complete_badge() {
  Texture sprites = ui_get_sprites();
  Rectangle tgt = {200, 200, 100, 100};
  DrawTexturePro(sprites, rect_medal, tgt, (Vector2){0, 0}, 0, CA_WHITE);
}

static bool get_can_rewind() { return C.api.bw != NULL; }

void win_main_draw() {
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

  if (C.mode == MODE_SIMU) {
    bool complete = C.sim.complete;
    if (complete) {
      // draw_complete_badge();
    }
  }

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

  btn_draw_text(&C.btn_layer_push, "+");
  btn_draw_text(&C.btn_layer_pop, "-");
  btn_draw_text(&C.btn_layer[0], "F1");
  btn_draw_text(&C.btn_layer[1], "F2");
  btn_draw_text(&C.btn_layer[2], "F3");

  int al = hist_get_active_layer(&C.ca.h);
  int v0 = paint_get_layer_vis(&C.ca, 0);
  int v1 = paint_get_layer_vis(&C.ca, 1);
  int v2 = paint_get_layer_vis(&C.ca, 2);
  int v3 = paint_get_layer_vis(&C.ca, 3);
  btn_draw_icon(&C.btn_layer_v[0], v0 ? rect_eye_on : rect_eye_off);
  btn_draw_icon(&C.btn_layer_v[1], v1 ? rect_eye_on : rect_eye_off);
  btn_draw_icon(&C.btn_layer_v[2], v2 ? rect_eye_on : rect_eye_off);
  btn_draw_icon(&C.btn_layer_v[3], v3 ? rect_eye_on : rect_eye_off);

  btn_draw_icon(&C.btn_new, rect_new);
  btn_draw_icon(&C.btn_open, rect_open);
  btn_draw_icon(&C.btn_save, rect_save);
  btn_draw_icon(&C.btn_saveas, rect_saveas);
  btn_draw_icon(&C.btn_settings, rect_settings);
  btn_draw_icon(&C.btn_exit, rect_exit);

  btn_draw_icon(&C.btn_clockopt[0], rect_hz0);
  btn_draw_icon(&C.btn_clockopt[1], rect_hz1);
  btn_draw_icon(&C.btn_clockopt[2], rect_hz4);
  btn_draw_icon(&C.btn_clockopt[3], rect_hz16);
  btn_draw_icon(&C.btn_clockopt[4], rect_hz64);
  btn_draw_icon(&C.btn_clockopt[5], rect_hz1k);
  btn_draw_icon(&C.btn_sim_show_t, rect_inspect_wire);
  btn_draw_icon(&C.btn_sim_soladd, rect_soladd);

  int mode = main_get_simu_mode();
  bool simu_on = mode == MODE_SIMU || mode == MODE_ERROR;

  Rectangle rec_simu = {0};
  if (simu_on) rec_simu = rect_stop;
  if (!simu_on) rec_simu = rect_start;
  if (C.kernel_error) rec_simu = rect_warning;
  btn_draw_icon(&C.btn_simu, rec_simu);
  btn_draw_icon(&C.btn_rewind, rect_rewind);
  btn_draw_icon(&C.btn_forward, rect_forward);
  btn_draw_icon(&C.btn_pause, rect_pause);

  btn_draw_text(&C.btn_wiki, T.main_btn_wiki);
  btn_draw_text(&C.btn_level_custom, T.main_select_level_custom);
  btn_draw_text(&C.btn_level_campaign, T.main_select_level);

  bool color_disabled = mode != MODE_EDIT;
  btn_draw_color(C.fg_color_rect, paint_get_color(&C.ca), false,
                 color_disabled);
  for (int i = 0; i < C.num_colors; i++) {
    btn_draw_color(C.color_btn[i], C.palette[i],
                   COLOR_EQ(C.palette[i], paint_get_color(&C.ca)),
                   color_disabled);
  }
  btn_draw_icon(&C.btn_line, rect_line);
  btn_draw_text(&C.btn_blueprint, T.main_btn_blueprint);
  btn_draw_icon(&C.btn_brush, rect_brush);
  btn_draw_icon(&C.btn_picker, rect_picker);
  btn_draw_icon(&C.btn_bucket, rect_bucket);
  btn_draw_icon(&C.btn_marquee, rect_marquee);
  btn_draw_icon(&C.btn_text, rect_text);
  btn_draw_icon(&C.btn_rotate, rect_rot);
  btn_draw_icon(&C.btn_fliph, rect_fliph);
  btn_draw_icon(&C.btn_flipv, rect_flipv);
  btn_draw_icon(&C.btn_fill, rect_fill);
  btn_draw_icon(&C.btn_line_sep, rect_line_sep);
  btn_draw_icon(&C.btn_line_sep_r, rect_line_sep_r);
  btn_draw_icon(&C.btn_sel_open, rect_sel_open);
  btn_draw_icon(&C.btn_sel_save, rect_sel_save);
  btn_draw_icon(&C.btn_blueprint_add, rect_blueprint_add);

  // We only draw the legends if this window is the active window.
  if (ui_get_window() == WINDOW_MAIN) {
    btn_draw_legend(&C.btn_settings, T.main_settings_leg);
    btn_draw_legend(&C.btn_new, T.main_new_leg);
    btn_draw_legend(&C.btn_open, T.main_open_leg);
    btn_draw_legend(&C.btn_save, T.main_save_leg);
    btn_draw_legend(&C.btn_saveas, T.main_saveas_leg);
    btn_draw_legend(&C.btn_blueprint, T.main_btn_blueprint_leg);
    btn_draw_legend(&C.btn_exit, T.main_exit_leg);

    btn_draw_legend(&C.btn_layer_push, T.main_layer_push_leg);
    btn_draw_legend(&C.btn_layer_pop, T.main_layer_pop_leg);

    for (int i = 0; i < MAX_LAYERS; i++) {
      btn_draw_legend(&C.btn_layer_v[i],
                      TextFormat(T.main_layer_show_leg, i + 1));
    }

    btn_draw_legend(&C.btn_layer[0], T.main_layer_f1_leg);
    btn_draw_legend(&C.btn_layer[1], T.main_layer_f2_leg);
    btn_draw_legend(&C.btn_layer[2], T.main_layer_f3_leg);

    btn_draw_legend(&C.btn_level_custom, T.main_select_level_custom_leg);
    btn_draw_legend(&C.btn_level_campaign, T.main_select_level_leg);
    btn_draw_legend(&C.btn_wiki, T.main_wiki_leg);

    btn_draw_legend(&C.btn_sel_open, T.main_open_sel_leg);
    btn_draw_legend(&C.btn_sel_save, T.main_save_sel_leg);
    btn_draw_legend(&C.btn_blueprint_add, T.main_bp_add_leg);

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
    btn_draw_legend(
        &C.btn_clockopt[4],
        TextFormat(T.main_speed5_leg, fmtnum((int)(get_speed_dt(4)))));
    btn_draw_legend(
        &C.btn_clockopt[5],
        TextFormat(T.main_speed6_leg, fmtnum((int)(get_speed_dt(5)))));

    btn_draw_legend(&C.btn_sim_show_t, T.main_show_start_leg);
    btn_draw_legend(&C.btn_sim_soladd, T.main_soladd_leg);

    if (!C.kernel_error) {
      btn_draw_legend(&C.btn_simu,
                      simu_on ? T.main_simu_stop_leg : T.main_simu_start_leg);
    } else {
      btn_draw_legend(&C.btn_simu, T.main_simu_err_leg);
    }

    btn_draw_legend(&C.btn_pause, T.main_pause_leg);
    bool rewind_disabled = !get_can_rewind();
    if (rewind_disabled) {
      btn_draw_legend(&C.btn_rewind, T.main_rewind_disabled_leg);
    } else {
      btn_draw_legend(&C.btn_rewind, T.main_rewind_leg);
    }
    btn_draw_legend(&C.btn_forward, T.main_forward_leg);
    btn_draw_legend(&C.btn_brush, T.main_brush_leg);
    btn_draw_legend(&C.btn_line, T.main_line_leg);
    btn_draw_legend(&C.btn_bucket, T.main_bucket_leg);
    btn_draw_legend(&C.btn_picker, T.main_picker_leg);
    btn_draw_legend(&C.btn_marquee, T.main_marquee_leg);
    btn_draw_legend(&C.btn_text, T.main_text_leg);
    btn_draw_legend(&C.btn_fliph, T.main_fliph_leg);
    btn_draw_legend(&C.btn_flipv, T.main_flipv_leg);
    btn_draw_legend(&C.btn_rotate, T.main_rot_leg);
    btn_draw_legend(&C.btn_fill, T.main_fill_leg);
    btn_draw_legend(&C.btn_line_sep, T.main_linesep_leg);
    btn_draw_legend(&C.btn_line_sep_r, T.main_linesep_reset_leg);
  }

  if (ui_get_window() == WINDOW_MAIN) {
    main_draw_mouse_extra();
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

static bool can_save_as_solution() {
  bool has_lvl = get_level_id() != NULL;
  return (C.mode == MODE_SIMU) && C.sim.complete && has_lvl;
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
  draw_text_box(msg, (Rectangle){pad, pad + dy, 1000, 0}, CA_WHITE, NULL);
  rlPopMatrix();
}

void main_draw_status_bar() {
  Color tc = CA_WHITE;
  Color bg = BLACK;
  int lh = uifont_line_height();
  int step = lh + 2;
  int num_lines = 11;
  int xc = 8 + C.target_pos.x;
  int ybot = C.target_pos.y + C.img_target_tex.texture.height;
  int yc1 = ybot - step * num_lines;
  int yc2 = yc1 + step;
  int yc3 = yc2 + step;
  int yc4 = yc3 + step;
  int yc5 = yc4 + step;
  int yc6 = yc5 + step;
  int yc7 = yc6 + step;
  int yc8 = yc7 + step;
  int yc9 = yc8 + step;
  int yc10 = yc9 + step;

  char txt[500];
  Paint* pnt = &C.ca;
  if (pnt->mouseOnTarget) {
    v2i cursor = paint_get_cursor(pnt);
    snprintf(txt, sizeof(txt), T.main_bar_coord, cursor.x, cursor.y);
    uifont_draw_texture_outlined(txt, xc, yc1, tc, bg);
    if (main_get_simu_mode() == MODE_EDIT) {
      v2i selSize = paint_get_active_sel_size(pnt);
      int sw = selSize.x;
      int sh = selSize.y;
      if (sw >= 0) {
        snprintf(txt, sizeof(txt), T.main_bar_sel, sw, sh);
        uifont_draw_texture_outlined(txt, xc, yc2, tc, bg);
      }
    }
    int zoomPerc = paint_get_zoom_perc(pnt);
    snprintf(txt, sizeof(txt), T.main_bar_zoom, zoomPerc);
    uifont_draw_texture_outlined(txt, xc, yc3, tc, bg);
  }

  int yc = yc4;
  if (main_get_simu_mode() == MODE_SIMU) {
    int num_nands = arrlen(C.sim.pg.nands);
    snprintf(txt, sizeof(txt), "N: %s", fmtnum(num_nands));
    uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
    yc += step;

    bool warmup = sim_is_on_warmup(&C.sim);
    if (warmup) {
      snprintf(txt, sizeof(txt), "...");
      // E, C, T, CRIT:
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;
    } else {
      snprintf(txt, sizeof(txt), T.main_bar_energy,
               fmtnum((int)(C.sim.state.total_energy)));
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;
      int cycle = sim_get_effective_cycle(&C.sim);

      snprintf(txt, sizeof(txt), "C: %d", cycle);
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;

      int t = C.sim.state.cur_period_tick + 1;
      snprintf(txt, sizeof(txt), "T: %d", t);
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;

      snprintf(txt, sizeof(txt), "CRIT: C: %d T: %d",
               C.sim.state.max_tick_cycle, C.sim.state.max_tick);
      uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
      yc += step;
    }
  } else {
    yc += step * 5;
  }

  if (!C.bp) {
    const char* fname = GetFileName(get_filename());
    snprintf(txt, sizeof(txt), T.main_bar_img_name, fname);
    uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
  } else {
    // snprintf(txt, sizeof(txt), "[blueprint] %s", C.bp->name);
    snprintf(txt, sizeof(txt), T.main_bar_img_name, C.bp->name);
    uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
  }
  yc += step;
  v2i buf_size = hist_get_buf_size(&C.ca.h);
  snprintf(txt, sizeof(txt), T.main_bar_img, buf_size.x, buf_size.y);
  uifont_draw_texture_outlined(txt, xc, yc, tc, bg);
  yc += step;

  const char* lvl = NULL;
  if (C.ldef) {
    lvl = C.ldef->name;
  } else if (C.cldef) {
    lvl = C.cldef->name;
  }
  const char* line = TextFormat(T.main_bar_level, lvl);
  uifont_draw_texture_outlined(line, xc, yc, tc, bg);
  yc += step;
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
    bool just_changed = paint_get_key_line_width_has_just_changed(&C.ca);
    char txt[50];
    int lh = uifont_line_height();
    int tx = (int)pos.x + 16;
    int ty = (int)pos.y + 16;
    snprintf(txt, sizeof(txt), T.main_cursor_width,
             paint_get_line_width(&C.ca));
    uifont_draw_texture(txt, tx, ty, (just_changed ? YELLOW : CA_WHITE));
    int sep = paint_get_line_sep(&C.ca);
    if (sep != 1) {
      snprintf(txt, sizeof(txt), T.main_cursor_sep, sep);
      uifont_draw_texture(txt, tx, ty + lh + 2, CA_WHITE);
    }
  }
  if (strlen(C.mouse_msg) > 0) {
    Vector2 pos = GetMousePosition();
    Color msg_color = BLANK;
    if (C.mouse_msg_type == 0) msg_color = CA_WHITE;
    if (C.mouse_msg_type == 1) msg_color = RED;
    uifont_draw_texture_outlined(C.mouse_msg, (int)pos.x + 16, (int)pos.y + 16,
                                 msg_color, BLACK);
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

  bool can_rewind = main_can_rewind();
  C.btn_new.disabled = ned;
  C.btn_open.disabled = ned || demo;

  C.btn_brush.disabled = ned;
  C.btn_line.disabled = ned;
  C.btn_marquee.disabled = ned;
  C.btn_text.disabled = ned;
  C.btn_bucket.disabled = ned;
  C.btn_picker.disabled = ned;

  C.btn_pause.toggled = C.paused;
  C.btn_rewind.toggled = C.rewind_pressed;
  C.btn_forward.toggled = C.forward_pressed;

  C.btn_rewind.disabled = !ned || !can_rewind;
  C.btn_forward.disabled = !ned || !C.paused;
  C.btn_pause.disabled = !ned;

  C.btn_simu.disabled = C.kernel_error;

  C.btn_level_campaign.disabled = ned;
  C.btn_level_custom.disabled = ned;
  C.btn_blueprint.disabled = ned;
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
  // C.btn_blueprint_add.hidden = (tool != TOOL_SEL) || ned;
  // C.btn_blueprint.hidden = (tool != TOOL_SEL) || ned;

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

  // C.btn_sim_soladd.hidden = C.mode != MODE_SIMU;
  C.btn_sim_soladd.disabled = C.mode != MODE_SIMU || !can_save_as_solution();

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
  C.bp = NULL;
  paint_new_buffer(&C.ca);
  update_title();
}

void win_main_destroy() {
  discord_shutdown();
  paint_destroy(&C.ca);
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  UnloadRenderTexture(C.img_target_tex);
}

static void load_image_from_path_ex(const char* path, bool keep_file) {
  paint_load_image(&C.ca, LoadImage(path));
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  if (keep_file) {
    C.fname = clone_string(path);
  }
  update_title();
}

void win_main_load_blueprint(Blueprint* bp) {
  bool keep_filename = bp->steam_id == 0;
  load_image_from_path_ex(blueprint_fname_full(bp), keep_filename);
  if (bp->linked_level_id) {
    main_load_by_level_id(bp->linked_level_id);
  }
  win_main_update(); /* To refresh the canva's content */
  if (keep_filename) {
    win_msg_open_text(T.main_blueprint_associated, NULL);
    C.bp = bp;
  } else {
    win_msg_open_text(T.main_blueprint_not_associated, NULL);
  }
}

void win_main_load_image_from_path(const char* path) {
  bool keep_filename = true;
  C.bp = NULL;
  load_image_from_path_ex(path, keep_filename);
}

void main_open_file_modal() {
  on_modal_before_open();
  ModalResult mr = modal_open_file(NULL);
  on_modal_after_open();
  if (mr.ok) {
    win_main_load_image_from_path(mr.fPath);
    free(mr.fPath);
  } else if (mr.cancel) {
  } else {
    char txt[500];
    snprintf(txt, sizeof(txt), "ERROR: %s\n", mr.errMsg);
    msg_add(txt, MSG_DURATION);
  }
}

static void on_dialog_close(int r) {
  if (r == 2 || r == -1) {  // cancel: no action
    return;
  }
  if (r == 0) {  // save
    if (on_save_click(false)) {
      return;  // Cancelled during save
    }
  }
  // Exectued on resul=0 or result=1
  C.dialog_callback();
}

void win_main_ask_for_save_and_proceed(void (*next_action)()) {
  C.dialog_callback = next_action;
  if (!paint_get_is_dirty(&C.ca)) {
    next_action();
  } else {
    dialog_open(T.want_to_save_changes, T.save, T.dont_save, T.cancel,
                on_dialog_close);
  }
}

void win_main_set_line_sep(int n) {
  if (n <= 1) n = 1;
  if (n >= 128) n = 128;
  paint_set_line_sep(&C.ca, n);
}

void win_main_paste_file(const char* fname, int rot) {
  Image img = LoadImage(fname);
  if (img.width == 0) {
    msg_add(TextFormat(T.main_could_not_open_image, fname), 10);
    return;
  }
  paint_paste_image(&C.ca, img, rot);
  // msg_add(TextFormat("Pasted %s", fname), 3);
}

Blueprint* win_main_get_editting_blueprint() { return C.bp; }

bool win_main_is_simu_error() {
  if (C.mode != MODE_SIMU) return true;
  return sim_has_errors(&C.sim);
}

bool win_main_is_simu_done() { return can_save_as_solution(); }
