#include "win_settings.h"

// #include "i18n.h"
#include "i18n.h"
#include "layout.h"
#include "raylib.h"
#include "stb_ds.h"
#include "stdio.h"
#include "ui.h"
#include "utils.h"
#include "wabout.h"
#include "widgets.h"
#include "win_main.h"

#define WIN_W 600
#define WIN_H 400

static struct {
  Rectangle modal;
  Layout* layout;
  Btn btn_always_on_top;
  Btn btn_neon;
  Btn btn_paint_sound;
  Btn btn_simu_sound;
  Btn btn_close;
  Btn btn_about;
  bool opened_this_frame;
} C = {0};

static void update_layout() {
  layout_update_offset(C.layout);
  Layout* l = C.layout;
  C.modal = layout_rect(l, "window");
  C.btn_always_on_top.hitbox = layout_rect(l, "always_on_top");
  C.btn_neon.hitbox = layout_rect(l, "neon");
  C.btn_paint_sound.hitbox = layout_rect(l, "paint_sound");
  C.btn_simu_sound.hitbox = layout_rect(l, "simu_sound");
  C.btn_about.hitbox = layout_rectb(l, "btn_about");
  C.btn_close.hitbox = layout_rectb(l, "btn_close");
}

void win_settings_init() { C.layout = easy_load_layout("settings"); }

void win_settings_open() {
  ui_winpush(WINDOW_SETTINGS);
  update_layout();
  C.opened_this_frame = true;
}

static void update_options() {
  GameRegistry* r = getreg();

  if (btn_update(&C.btn_neon)) {
    r->cfg.simu_neon = !r->cfg.simu_neon;
    save_progress();
  }

  if (btn_update(&C.btn_always_on_top)) {
    r->cfg.always_on_top = !r->cfg.always_on_top;
    on_always_on_top_change();
    save_progress();
  }

  if (btn_update(&C.btn_simu_sound)) {
    r->cfg.simu_sound = !r->cfg.simu_sound;
    save_progress();
  }

  if (btn_update(&C.btn_paint_sound)) {
    r->cfg.paint_sound = !r->cfg.paint_sound;
    save_progress();
  }

  C.btn_neon.toggled = r->cfg.simu_neon;
  C.btn_always_on_top.toggled = r->cfg.always_on_top;
  C.btn_simu_sound.toggled = r->cfg.simu_sound;
  C.btn_paint_sound.toggled = r->cfg.paint_sound;

  bool hover = false;
  hover |= C.btn_neon.hover;
  hover |= C.btn_always_on_top.hover;
  hover |= C.btn_simu_sound.hover;
  hover |= C.btn_paint_sound.hover;
  if (hover) ui_set_cursor(MOUSE_POINTER);
}

void win_settings_update() {
  update_layout();
  update_options();
  if (btn_update(&C.btn_about)) {
    easy_about_open();
  }
  if (C.opened_this_frame) {
    C.opened_this_frame = false;
    return;
  }
  if (IsKeyPressed(KEY_ESCAPE) || btn_update(&C.btn_close)) {
    ui_winpop();
    return;
  }
}

void win_settings_draw() {
  draw_win(C.modal, T.settings_title);
  btn_draw_checkbox_text(&C.btn_neon, T.settings_wire_glow);
  btn_draw_checkbox_text(&C.btn_always_on_top, T.settings_always_on_top);
  btn_draw_checkbox_text(&C.btn_paint_sound, T.settings_drawing_sound);
  btn_draw_checkbox_text(&C.btn_simu_sound, T.settings_nand_sound);
  Texture sprites = ui_get_sprites();
  btn_draw_icon(&C.btn_about, rect_info);
  btn_draw_text(&C.btn_close, T.close);
  if (ui_get_window() == WINDOW_SETTINGS) {
    btn_draw_legend(&C.btn_about, T.settings_about_leg);
  }
}

