#include "w_cpedia.h"

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

void CpediaUpdateLayout() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int lw = 4 * 35 * 2;
  int bw = 12 * 35 * 2;
  int total_w = lw + bw + 2 * 2;
  int x = (sw - total_w) / 2;
  int bh = 9 * 35 * 2;
  int s = 2;
  int total_h = (bh + 35 * 2 + 35 * 2);
  int y = (sh - total_h) / 2;
  int yy = y + 35 * 2;
  C.title = (Rectangle){x, y, total_w, 35 * 2};
  int pad = 10 * s;
  C.modal = (Rectangle){x - pad, y - pad, total_w + 2 * pad, total_h + 2 * pad};
  Rectangle lb_box = {x, yy, lw, bh};
  Rectangle box = {x + lw + 2 * s, yy, bw, bh};
  C.textbox_wrap = box;
  Rectangle box2 = box;
  box2.x = box.x + s;
  box2.y = box.y + 4 * s;
  box2.height = box.height - 2 * 4 * s;
  box2.width = box.width - 2 * s;
  int bsize = 4 * 35 * s;
  C.btn_close.hitbox = (Rectangle){
      box.x + box.width - bsize,
      box.y + box.height + 12 * 2,
      bsize,
      17 * 2,
  };
  TextboxSetBox(&C.tb, box2);
  ListboxSetBox(&C.lb, lb_box);
}

void CpediaOpen(Ui* ui) {
  ui->window = WINDOW_CPEDIA;
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

void CpediaUpdate(Ui* ui) {
  CpediaUpdateLayout();
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

void CpediaDraw(Ui* ui) {
  int sh = GetScreenHeight();
  int sw = GetScreenWidth();
  Color bg = BLACK;
  bg.a = 150;
  DrawRectangle(0, 0, sw, sh, bg);
  const char* title = "CIRCUITOPEDIA";
  Vector2 ts = GetRenderedTextSize(title);
  int th = ts.y * 4;
  int tw = ts.x * 4;
  int yy = (C.title.height - th) / 2;
  int xx = (C.title.width - tw) / 2;
  BeginScissorMode(C.modal.x, C.modal.y, C.modal.width, C.modal.height);
  DrawDefaultTiledScreen(ui);
  EndScissorMode();
  rlPushMatrix();
  rlTranslatef(C.title.x + xx, C.title.y + yy, 0);
  rlScalef(4, 4, 1);
  FontDrawTexture(title, 1, 1, BLACK);
  FontDrawTexture(title, 0, 0, WHITE);
  rlPopMatrix();
  DrawRectangleRec(C.textbox_wrap, BLACK);
  TextboxDraw(&C.tb, ui);
  ListboxDraw(&C.lb, ui, C.sel);
  BtnDrawText(&C.btn_close, ui->scale, "CLOSE");
  DrawDefaultTiledFrame(ui, C.modal);
}
