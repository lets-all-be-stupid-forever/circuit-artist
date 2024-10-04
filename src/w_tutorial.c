#include "w_tutorial.h"

#include "api.h"
#include "font.h"
#include "raylib.h"
#include "rlgl.h"
#include "stb_ds.h"
#include "tiling.h"
#include "ui.h"
#include "widgets.h"

static struct {
  bool inited;
  Textbox tb;
  Rectangle textbox_wrap;
  Listbox lb;
  int sel;
  Rectangle title;
  Rectangle modal;
  bool opened_this_frame;
  bool closed;
  Btn btn_close;
} C = {0};

void TutorialUpdateLayout() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int lw = 4 * 35 * 2;
  int bw = 12 * 35 * 2;
  int total_w = lw + bw + 2 * 2;
  int x = (sw - total_w) / 2;
  int bh = 10 * 35 * 2;
  int s = 2;
  int pad = 10 * s;
  int total_h = (bh + 35 * 1 + 35 * 1);
  while (total_h + 2 * pad > sh) {
    bh -= 35 * 2;
    total_h -= 35 * 2;
  }

  bh -= 4 * s;
  int y = (sh - total_h) / 2;
  int yy = y + 35 * 1;
  C.title = (Rectangle){x, y, total_w, 35 * 2};
  C.modal = (Rectangle){x - pad, y - pad, total_w + 2 * pad, total_h + 2 * pad};
  Rectangle lb_box = {x, yy, lw, bh};
  Rectangle box = {x + lw + 2 * s + 6 * s, yy, bw - 8 * s, bh};
  C.textbox_wrap = box;
  Rectangle box2 = box;
  box2.x = box.x;
  // box2.y = box.y + 4 * s;
  // box2.height = box.height - 2 * 4 * s;
  box2.width = box.width;
  int bsize = 4 * 35 * s;
  C.btn_close.hitbox = (Rectangle){
      box.x + box.width - bsize + 2 * s,
      box.y + box.height + 8 * 2,
      bsize,
      17 * 2,
  };
  TextboxSetBox(&C.tb, box2);
  ListboxSetBox(&C.lb, lb_box);
}

void TutorialOpen(Ui* ui) {
  ui->window = WINDOW_TUTORIAL;
  C.opened_this_frame = true;
  C.closed = false;
  if (!C.inited) {
    C.inited = true;
    C.sel = 0;
    TextboxLoad(&C.tb);
    LevelOptions* co = ApiGetLevelOptions();
    TextboxSetContent(&C.tb, co->help_txt[C.sel], co->help_sprites[C.sel]);
    ListboxLoad(&C.lb);
    for (int i = 0; i < 50; i++) {
      if (co->help_name[i] == NULL) {
        break;
      }
      ListboxAddRow(&C.lb, co->help_name[i]);
    }
  }
}

void TutorialUpdate(Ui* ui) {
  TutorialUpdateLayout();
  if (IsKeyPressed(KEY_TAB) && !C.opened_this_frame) {
    ui->window = WINDOW_MAIN;
    return;
  }
  C.opened_this_frame = false;

  // Cancel button: we close without performing any action.
  if (BtnUpdate(&C.btn_close, ui) || IsKeyPressed(KEY_ESCAPE)) {
    ui->window = WINDOW_MAIN;
    return;
  }

  ListboxUpdate(&C.lb);
  TextboxUpdate(&C.tb, ui);
  int hit = C.lb.row_hit;
  if (hit >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && C.sel != hit) {
    C.sel = hit;
    LevelOptions* co = ApiGetLevelOptions();
    TextboxSetContent(&C.tb, co->help_txt[C.sel], co->help_sprites[C.sel]);
  }
}

void TutorialDraw(Ui* ui) {
  int sh = GetScreenHeight();
  int sw = GetScreenWidth();
  Color bg = BLACK;
  bg.a = 150;
  DrawRectangle(0, 0, sw, sh, bg);
  BeginScissorMode(C.modal.x, C.modal.y, C.modal.width, C.modal.height);
  DrawDefaultTiledScreen(ui);
  EndScissorMode();
  DrawRectangleRec(C.textbox_wrap, BLACK);
  DrawWidgetFrame(ui, C.textbox_wrap);
  TextboxDraw(&C.tb, ui);
  ListboxDraw(&C.lb, ui, C.sel);
  BtnDrawText(&C.btn_close, ui->scale, "CLOSE");
  DrawWidgetFrameInv(ui, C.modal);
  DrawTitle(ui, C.modal, "TUTORIAL");
}
