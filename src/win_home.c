#include "win_home.h"

#include "layout.h"
#include "ui.h"
#include "utils.h"
#include "win_campaign.h"
#include "win_customlvl.h"
#include "win_level.h"
#include "win_main.h"

static struct {
  Layout* layout;
  Rectangle modal;
  Btn btn_camp;
  Btn btn_custom;
  Btn btn_exit;
} C = {0};

static void update_layout() {
  layout_update_offset(C.layout);
  Layout* l = C.layout;
  C.modal = layout_rect(l, "window");
  C.btn_camp.hitbox = layout_rectb(l, "btn_camp");
  C.btn_custom.hitbox = layout_rectb(l, "btn_custom");
  C.btn_exit.hitbox = layout_rectb(l, "btn_exit");
}

void win_home_init() {
  C.layout = easy_load_layout("home");
  update_layout();
}

void win_home_open() {
  ui_winpush(WINDOW_HOME);
  update_layout();
}

void win_home_update() {
  if (btn_update(&C.btn_exit)) {
    ui_set_close_requested();
    return;
  }
  if (btn_update(&C.btn_camp)) {
    ui_winpop();
    win_main_open(find_sandbox_custom_level());
  }
  if (btn_update(&C.btn_custom)) {
    ui_winpop();
    win_main_open(find_sandbox_custom_level());
  }
}

void win_home_draw() {
  ClearBackground(BLANK);
  draw_default_tiled_screen();
  draw_win(C.modal, "CIRCUIT ARTIST");
  btn_draw_text(&C.btn_camp, "CAMPAIGN");
  btn_draw_text(&C.btn_custom, "CUSTOM");
  btn_draw_text(&C.btn_exit, "EXIT");
}

