#include "win_workshopdet.h"

#include "i18n.h"
#include "layout.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"
#include "win_workshop.h"

static struct {
  Layout* layout;
  Rectangle modal;
  Btn btn_close;
  Btn btn_subscribe;
  Textbox tb;
} C = {0};

static void update_layout() {
  layout_update_offset(C.layout);
  C.modal = layout_rect(C.layout, "window");
  textbox_set_box(&C.tb, layout_rect(C.layout, "desc"));
}

void win_workshopdet_init() {
  C.layout = easy_load_layout("workshopdet");
  textbox_init(&C.tb);
  update_layout();
}

void win_workshopdet_open() {
  ui_winpush(WINDOW_WORKSHOPDET);
  update_layout();
}

void win_workshopdet_update() {
  update_layout();
  if (IsKeyPressed(KEY_ESCAPE) || btn_update(&C.btn_close)) {
    ui_winpop();
    return;
  }

  if (btn_update(&C.btn_subscribe)) {
    // todo
  }
}

void win_workshopdet_draw() {
  draw_win(C.modal, T.workshop_title);
  btn_draw_text(&C.btn_close, T.close);
  C.btn_subscribe.primary = true;
  btn_draw_text(&C.btn_subscribe, T.workshopdet_subscribe);
}

void win_workshopdet_on_close() {
  // todo
}

