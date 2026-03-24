#include "win_mtext.h"

#include "ui.h"
#include "utils.h"
#include "widgets.h"

static struct {
  MultiLineEdit mle;
  Btn bClose;
  Btn bAccept;
  Rectangle modal;
  void* ctx;
  void (*on_accept)(void* ctx, const char* txt);
} C = {0};

static void update_layout() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int s = ui_get_scale();
  int pad = 12 * s;
  int mw = sw * 2 / 3;
  int mh = sh * 2 / 3;
  int mx = (sw - mw) / 2;
  int my = (sh - mh) / 2;
  C.modal = (Rectangle){mx, my, mw, mh};

  int title_h = 16 * s;
  int btn_h = 17 * s;
  int btn_w = 4 * 35 * s;
  int mle_x = mx + pad;
  int mle_y = my + title_h + pad;
  int mle_w = mw - 2 * pad;
  int mle_h = mh - title_h - btn_h - 3 * pad;

  mle_set_box(&C.mle, (Rectangle){mle_x, mle_y, mle_w, mle_h});

  C.bClose.hitbox = (Rectangle){
      mx + mw - btn_w - pad,
      my + mh - btn_h - pad,
      btn_w,
      btn_h,
  };
  C.bAccept.hitbox = (Rectangle){
      mx + mw - 2 * btn_w - 2 * pad,
      my + mh - btn_h - pad,
      btn_w,
      btn_h,
  };
}

void win_mtext_open(void (*on_accept)(void* ctx, const char* txt), void* ctx,
                    const char* initial_txt) {
  ui_winpush(WINDOW_MTEXT);
  C.on_accept = on_accept;
  C.ctx = ctx;
  update_layout();
  mle_set_text(&C.mle, initial_txt ? initial_txt : "");
}

void win_mtext_update() {
  update_layout();
  if (btn_update(&C.bClose) || IsKeyPressed(KEY_ESCAPE)) {
    ui_winpop();
    return;
  }
  if (btn_update(&C.bAccept)) {
    if (C.on_accept) C.on_accept(C.ctx, mle_get_text(&C.mle));
    ui_winpop();
    return;
  }
  mle_update(&C.mle);
}

void win_mtext_draw() {
  draw_win(C.modal, "TEXT EDITOR");
  mle_draw(&C.mle);
  btn_draw_text(&C.bClose, ui_get_scale(), "CLOSE");
  btn_draw_text_primary(&C.bAccept, ui_get_scale(), "ACCEPT");
}

void win_mtext_init() {
  mle_init(&C.mle);
  // C.mle.readonly = true;
}
