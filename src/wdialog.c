#include "wdialog.h"

#include <stdlib.h>

#include "font.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"
#include "wmain.h"

static struct {
  Rectangle confirmModal;
  Btn btn_opt[3];
  char* msg;
  char* opt[3];
  void (*nxt_action)(int r);  // Next action for modal
} C = {0};

static void dialog_update_layout() {
  int nb = 0; /* num buttons*/
  if (C.opt[0]) nb++;
  if (C.opt[1]) nb++;
  if (C.opt[2]) nb++;

  int n = nb;
  int gap = 12;
  int rw = 120;
  int tot = (n - 1) * gap + n * rw;
  int rh = 17 * 2;

  int mW = 500;
  int mH = 180;
  int mX = (GetScreenWidth() - mW) / 2;
  int mY = (GetScreenHeight() - mH) / 2;
  C.confirmModal = (Rectangle){mX, mY, mW, mH};
  int x0 = mX + (mW - tot) / 2;
  int y0 = mY + (mH - 17 * 2 - 20);
  for (int i = 0; i < n; i++) {
    C.btn_opt[i].hitbox = (Rectangle){
        x0 + i * (gap + rw),
        y0,
        rw,
        rh,
    };
  }
}

static void reset() {
  free(C.msg);
  free(C.opt[0]);
  free(C.opt[1]);
  free(C.opt[2]);
  C.msg = NULL;
  C.opt[0] = NULL;
  C.opt[1] = NULL;
  C.opt[2] = NULL;
}

void dialog_open(const char* modal_msg, const char* opt0, const char* opt1,
                 const char* opt2, void (*nxt_action)(int r)) {
  reset();
  ui_winpush(WINDOW_DIALOG);
  C.msg = clone_string(modal_msg);
  C.nxt_action = nxt_action;
  for (int i = 0; i < 3; i++) {
    C.btn_opt[i].pressed = false;
    C.btn_opt[i].disabled = false;
    C.btn_opt[i].toggled = false;
  }
  if (opt0) C.opt[0] = clone_string(opt0);
  if (opt1) C.opt[1] = clone_string(opt1);
  if (opt2) C.opt[2] = clone_string(opt2);

  C.btn_opt[0].hidden = !opt0;
  C.btn_opt[1].hidden = !opt1;
  C.btn_opt[2].hidden = !opt2;
}

void dialog_update() {
  dialog_update_layout();
  int result = -2;
  if (C.opt[0] && btn_update(&C.btn_opt[0])) result = 0;
  if (C.opt[1] && btn_update(&C.btn_opt[1])) result = 1;
  if (C.opt[2] && btn_update(&C.btn_opt[2])) result = 2;
  if (IsKeyPressed(KEY_ESCAPE)) result = -1;
  if (result == -2) {  // nothing
    return;
  } else {
    C.nxt_action(result);
    ui_winpop();
  }
}

void dialog_draw() {
  int s = ui_get_scale();
  Rectangle r = C.confirmModal;
  draw_win(r, "Confirm");
  int msg_size = get_rendered_text_size(C.msg).x;
  rlPushMatrix();
  rlTranslatef(r.x, r.y, 0);
  rlScalef(s, s, 1);
  int mx = (r.width / s - msg_size) / 2;
  int my = 36;
  font_draw_texture(C.msg, mx + 1, my + 1, BLACK);
  font_draw_texture(C.msg, mx, my, WHITE);
  rlPopMatrix();
  if (C.opt[0]) btn_draw_text(&C.btn_opt[0], s, C.opt[0]);
  if (C.opt[1]) btn_draw_text(&C.btn_opt[1], s, C.opt[1]);
  if (C.opt[2]) btn_draw_text(&C.btn_opt[2], s, C.opt[2]);
}
