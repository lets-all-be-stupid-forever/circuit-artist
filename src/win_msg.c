#include "win_msg.h"

#include "rlgl.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

static struct {
  Rectangle modal;
  Rectangle inner_modal;
  int pad;
  char* txt;
  Texture2D tex;
  int scale;
  int text_w;
  Textbox tb;

} C = {0};

static void reset() { C.tex = (Texture){0}; }

static void update_layout_img() {
  int s = C.scale;
  int tw = C.tex.width * s;
  int th = C.tex.height * s;
  int pad = 20;
  C.pad = pad;
  Rectangle mod = {0, 0, tw + 2 * pad, th + 2 * pad};
  Vector2 off = find_modal_off(mod);
  C.modal = roff(off, mod);
  C.inner_modal = (Rectangle){
      C.modal.x + pad,
      C.modal.y + pad,
      tw,
      th,
  };
}

static void update_layout_text() {
  int s = C.scale;
  int tw = C.text_w;
  int th = C.tb.height;
  int maxh = 35 * 6 * 2;
  if (th > maxh) th = maxh;

  int pad = 20;
  C.pad = pad;
  Rectangle mod = {0, 0, tw + 2 * pad, th + 2 * pad};
  Vector2 off = find_modal_off(mod);
  C.modal = roff(off, mod);
  C.inner_modal = (Rectangle){
      C.modal.x + pad,
      C.modal.y + pad,
      tw,
      th,
  };
  textbox_set_box(&C.tb, C.inner_modal);
}

static void update_layout() {
  if (C.tex.width > 0) {
    update_layout_img();
  } else {
    update_layout_text();
  }
}

void win_msg_open_text(const char* txt, sprite_t* sprites) {
  reset();
  ui_winpush(WINDOW_MSG);
  C.text_w = 35 * 8 * 2;
  textbox_set_box(&C.tb, (Rectangle){0, 0, C.text_w, 0});
  textbox_set_content(&C.tb, txt, sprites);
  update_layout();
}

void win_msg_open_tex(Texture2D tex, int scale) {
  reset();
  ui_winpush(WINDOW_MSG);
  C.scale = scale;
  C.tex = tex;
  update_layout();
}

void win_msg_update() {
  update_layout();
  if (C.tex.width == 0) {
    textbox_update(&C.tb);
  }
  if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
      IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    ui_winpop();
    return;
  }
}

void win_msg_draw() {
  draw_win(C.modal, NULL);
  int x = C.inner_modal.x;
  int y = C.inner_modal.y;
  // Color c = BLACK;
  // c.a = 100;
  // DrawRectangleRec(C.inner_modal, c);
  if (C.tex.width > 0) {
    rlPushMatrix();
    int s = C.scale;
    rlTranslatef(x, y, 0);
    rlScalef(s, s, 1);
    DrawTexture(C.tex, 0, 0, WHITE);
    rlPopMatrix();
  } else {
    textbox_draw(&C.tb);
  }
}

