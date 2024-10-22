#include "w_dialog.h"

#include <rlgl.h>
#include <stdlib.h>

#include "font.h"
#include "tiling.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

static struct {
  Rectangle confirm_modal;
  Btn confirm_buttons[3];
  char* modal_msg;
  UiCallback modal_next_action;  // Next action for modal
} C = {0};

static void DialogUpdateLayout() {
  int n = 3;
  int gap = 12;
  int rw = 120;
  int tot = (n - 1) * gap + n * rw;
  int rh = 17 * 2;

  int modal_w = 500;
  int modal_h = 180;
  int modal_x = (GetScreenWidth() - modal_w) / 2;
  int modal_y = (GetScreenHeight() - modal_h) / 2;
  C.confirm_modal = (Rectangle){modal_x, modal_y, modal_w, modal_h};
  int x0 = modal_x + (modal_w - tot) / 2;
  int y0 = modal_y + (modal_h - 17 * 2 - 20);

  for (int i = 0; i < n; i++) {
    C.confirm_buttons[i].hitbox = (Rectangle){
        x0 + i * (gap + rw),
        y0,
        rw,
        rh,
    };
  }
}

void DialogOpen(Ui* ui, const char* modal_msg, UiCallback modal_next_action) {
  ui->window = WINDOW_DIALOG;
  if (C.modal_msg) free(C.modal_msg);
  C.modal_msg = CloneString(modal_msg);
  C.modal_next_action = modal_next_action;
  for (int i = 0; i < 3; i++) {
    C.confirm_buttons[i].pressed = false;
    C.confirm_buttons[i].disabled = false;
    C.confirm_buttons[i].toggled = false;
  }
}

void DialogUpdate(Ui* ui) {
  DialogUpdateLayout();
  int result = -1;
  if (BtnUpdate(&C.confirm_buttons[0], ui)) result = 0;
  if (BtnUpdate(&C.confirm_buttons[1], ui)) result = 1;
  if (BtnUpdate(&C.confirm_buttons[2], ui) || IsKeyPressed(KEY_ESCAPE))
    result = 2;
  if (result == -1) {  // nothing
    return;
  }
  if (result == 2) {  // cancel: no action
    ui->window = WINDOW_MAIN;
    return;
  }
  if (result == 0) {  // save
    if (MainOnSaveClick(ui, false)) {
      ui->window = WINDOW_MAIN;
      return;  // Cancelled during save
    }
  }
  // Exectued on resul=0 or result=1
  C.modal_next_action(ui);
  ui->window = WINDOW_MAIN;
}

void DialogDraw(Ui* ui) {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  DrawRectangle(0, 0, sw, sh, (Color){.r = 0, .g = 0, .b = 0, .a = 150});
  int s = ui->scale;
  Rectangle r = C.confirm_modal;
  BeginScissorMode(C.confirm_modal.x, C.confirm_modal.y, C.confirm_modal.width,
                   C.confirm_modal.height);
  DrawDefaultTiledScreen(ui);
  EndScissorMode();

  int msg_size = GetRenderedTextSize(C.modal_msg).x;
  rlPushMatrix();
  rlTranslatef(r.x, r.y, 0);
  rlScalef(s, s, 1);
  int mx = (r.width / s - msg_size) / 2;
  int my = 36;
  FontDrawTexture(C.modal_msg, mx + 1, my + 1, BLACK);
  FontDrawTexture(C.modal_msg, mx, my, WHITE);
  rlPopMatrix();
  BtnDrawText(&C.confirm_buttons[0], s, "Save");
  BtnDrawText(&C.confirm_buttons[1], s, "Don't save");
  BtnDrawText(&C.confirm_buttons[2], s, "Cancel");
  DrawWidgetFrameInv(ui, C.confirm_modal);
  DrawTitle(ui, C.confirm_modal, "Confirm");
}
