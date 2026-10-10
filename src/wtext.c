#include "wtext.h"

#include "common.h"
#include "i18n.h"
#include "layout.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

static struct {
  Layout* layout;
  LineEdit edit;
  Btn btn_close;
  Rectangle modal;
  void* ctx;
  void (*on_accept)(void* ctx, const char* txt);
} C = {0};

static void update_layout() {
  layout_update_offset(C.layout);
  C.modal = layout_rect(C.layout, "window");
  C.edit.hitbox = layout_rectb(C.layout, "edit");
  C.btn_close.hitbox = layout_rectb(C.layout, "btn_close");
}

void text_modal_init() {
  C.layout = easy_load_layout("wtext");
  lineedit_init(&C.edit);
}

void text_modal_open(void (*on_accept)(void* ctx, const char* txt), void* ctx,
                     const char* initial_text) {
  ui_winpush(WINDOW_TEXT);
  C.on_accept = on_accept;
  C.ctx = ctx;
  update_layout();
  lineedit_set_text(&C.edit, initial_text ? initial_text : "");
  lineedit_set_focus(&C.edit, true);
}

void text_modal_update() {
  update_layout();

  if (btn_update(&C.btn_close) || IsKeyPressed(KEY_ESCAPE)) {
    lineedit_set_focus(&C.edit, false);
    ui_winpop();
    return;
  }

  if (IsKeyPressed(KEY_ENTER)) {
    if (C.on_accept) C.on_accept(C.ctx, lineedit_get_text(&C.edit));
    lineedit_set_focus(&C.edit, false);
    ui_winpop();
    return;
  }

  bool click_outside = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                        IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) &&
                       !CheckCollisionPointRec(GetMousePosition(), C.modal);
  if (click_outside) {
    lineedit_set_focus(&C.edit, false);
    ui_winpop();
    return;
  }

  lineedit_update(&C.edit);
}

void text_modal_draw() {
  draw_win(C.modal, T.main_insert_text);
  lineedit_draw(&C.edit);
  btn_draw_icon(&C.btn_close, rect_close);
}
