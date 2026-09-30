#include "win_progress.h"

#include "raylib.h"
#include "stdlib.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

static struct {
  ProgressCtx pc;
  Rectangle modal;
  Rectangle inner_modal;
  int pad;
  char* txt;
  int scale;
  int text_w;
  Textbox tb;
} C = {0};

static void update_layout() {
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

void win_progress_init() {
  C.text_w = 35 * 8 * 2;
  textbox_set_box(&C.tb, (Rectangle){0, 0, C.text_w, 0});
}

void win_progress_set_text(const char* txt) {
  if (C.txt) free(C.txt);
  C.txt = clone_string(txt);
  textbox_set_content(&C.tb, C.txt, NULL);
}

void win_progress_open(ProgressCtx pc) {
  ui_winpush(WINDOW_PROGRESS);
  C.pc = pc;
  update_layout();
}

void win_progress_update() {
  update_layout();
  bool should_close = C.pc.update(C.pc.ctx);
  if (should_close) {
    ui_winpop();
  }
  textbox_update(&C.tb);
}

void win_progress_draw() {
  draw_win(C.modal, NULL);
  int x = C.inner_modal.x;
  int y = C.inner_modal.y;
  // Color c = BLACK;
  // c.a = 100;
  // DrawRectangleRec(C.inner_modal, c);
  textbox_draw(&C.tb);
}

