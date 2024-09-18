#include "w_levels.h"

#include "api.h"
#include "font.h"
#include "rlgl.h"
#include "stdio.h"
#include "string.h"
#include "tiling.h"
#include "ui.h"
#include "widgets.h"

#define NUM_LEVEL_OPTS 16

static struct {
  bool inited;
  // Active selection
  int sel;
  Rectangle textbox_wrap;
  Rectangle modal;
  // Text description of the active selected level
  Textbox level_textbox;
  // one button for each challenge
  Btn btn_opts[NUM_LEVEL_OPTS];
  Rectangle leftside;
  Rectangle title;
  Btn btn_ok;
  Btn btn_cancel;
} C = {0};

static void LevelsUpdateLayout(Ui* ui) {
  int s = ui->scale;
  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();

  int h = 9;
  int bsize = 35 * s;

  int box_w = bsize * 12;
  int list_w = bsize * 5;
  int total_w = list_w + box_w + 2 * s;
  int x0 = (screen_width - total_w) / 2;
  int total_h = (h + 2) * bsize;
  int pad = 8 * s;
  while (total_h + 2 * pad + 10 > screen_height) {
    total_h -= bsize;
    h -= 1;
  }
  int y0 = (screen_height - total_h) / 2;
  C.title = (Rectangle){x0, y0, total_w, 35 * 2};
  C.modal = (Rectangle){
      x0 - pad,
      y0 - pad,
      total_w + 2 * pad,
      total_h + 2 * pad,
  };

  int yy = y0 + 35 * 2;
  for (int i = 0; i < NUM_LEVEL_OPTS; i++) {
    int xx = i % 4;
    int cy = i / 4;
    C.btn_opts[i].hitbox = (Rectangle){
        x0 + xx * 20 * 2 * s,
        yy + cy * 20 * 2 * s,
        35 * s,
        35 * s,
    };
  }
  C.leftside = (Rectangle){
      x0,
      yy,
      C.btn_opts[0].hitbox.width * 4,
      bsize * h,
  };
  Rectangle box = {
      x0 + list_w + 2 * s,
      yy,
      box_w,
      bsize * h,
  };
  C.textbox_wrap = box;
  Rectangle box2 = box;
  box2.x = box.x + s;
  box2.y = box.y + 4 * s;
  box2.height = box.height - 2 * 4 * s;
  box2.width = box.width - 2 * s;

  TextboxSetBox(&C.level_textbox, box2);

  int xb = box.x + box.width - 8 * bsize - 8 * s;
  int yb = box.height + box.y + 8 * s;
  C.btn_ok.hitbox = (Rectangle){xb, yb, 4 * bsize, 17 * s};
  C.btn_cancel.hitbox =
      (Rectangle){xb + 4 * bsize + 8 * s, yb, 4 * bsize, 17 * s};
}

static void LevelsSetSel(int ilevel) {
  C.sel = ilevel;
  // Sets selection number and updates textbox content.
  LevelOptions* co = ApiGetLevelOptions();
  const char* desc = co->options[ilevel].desc;
  TextboxSetContent(&C.level_textbox, desc, &co->options[C.sel].sprites[0]);
}

void LevelsOpen(Ui* ui) {
  ui->window = WINDOW_LEVELS;
  // If the options are not yet initialized, initialize them.
  if (!C.inited) {
    C.inited = true;
    TextboxLoad(&C.level_textbox);
    LevelOptions* co = ApiGetLevelOptions();
    for (int i = 0; i < 50; i++) {
      if (co->options[i].name == NULL) break;
    }
  }
  LevelsUpdateLayout(ui);
  // Sets the default selection to the selected level.
  LevelDesc* cd = ApiGetLevelDesc();
  LevelsSetSel(cd->ilevel);
}

void LevelsUpdate(Ui* ui) {
  // In case the window size has changed.
  LevelsUpdateLayout(ui);
  LevelOptions* co = ApiGetLevelOptions();
  for (int i = 0; i < NUM_LEVEL_OPTS; i++) {
    if (!co->options[i].name) {
      C.btn_opts[i].disabled = true;
    }
    if (BtnUpdate(&C.btn_opts[i], ui)) {
      LevelsSetSel(i);
    }
    C.btn_opts[i].toggled = i == C.sel;
  }

  // Cancel button: we close without performing any action.
  if (BtnUpdate(&C.btn_cancel, ui) || IsKeyPressed(KEY_ESCAPE)) {
    ui->window = WINDOW_MAIN;
    return;
  }

  // OK button: We load the new level (when applicable) in the API and set main
  // window as the active window.
  if (BtnUpdate(&C.btn_ok, ui) || IsKeyPressed(KEY_ENTER)) {
    LevelDesc* cd = ApiGetLevelDesc();
    // Only loads if the selected level is different from the active level.
    if (cd->ilevel != C.sel) {
      ApiLoadLevel(C.sel);
    }
    ui->window = WINDOW_MAIN;
    return;
  }

  TextboxUpdate(&C.level_textbox, ui);
}

void LevelsDraw(Ui* ui) {
  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();
  Color bg_color = BLACK;
  bg_color.a = 150;
  DrawRectangle(0, 0, screen_width, screen_height, bg_color);
  BeginScissorMode(C.modal.x, C.modal.y, C.modal.width, C.modal.height);
  DrawDefaultTiledScreen(ui);
  EndScissorMode();

  const char* title = "LEVEL SELECTION";
  Vector2 ts = GetRenderedTextSize(title);
  int th = ts.y * 4;
  int tw = ts.x * 4;
  int yy = (C.title.height - th) / 2;
  int xx = (C.title.width - tw) / 2;
  rlPushMatrix();
  rlTranslatef(C.title.x + xx, C.title.y + yy, 0);
  rlScalef(4, 4, 1);
  FontDrawTexture(title, 1, 1, BLACK);
  FontDrawTexture(title, 0, 1, BLACK);
  FontDrawTexture(title, 1, 0, BLACK);
  FontDrawTexture(title, 0, 0, WHITE);
  rlPopMatrix();

  LevelOptions* co = ApiGetLevelOptions();
  for (int i = 0; i < NUM_LEVEL_OPTS; i++) {
    if (co->options[i].name) {
      BtnDrawIcon(&C.btn_opts[i], ui->scale, co->options[i].icon.tex,
                  co->options[i].icon.region);
    }
  }
  BtnDrawText(&C.btn_ok, ui->scale, "CHOOSE LEVEL");
  BtnDrawText(&C.btn_cancel, ui->scale, "CANCEL");
  DrawRectangleRec(C.textbox_wrap, BLACK);
  DrawDefaultTiledFrame(ui, C.modal);
  TextboxDraw(&C.level_textbox, ui);
  for (int i = 0; i < NUM_LEVEL_OPTS; i++) {
    if (co->options[i].name) {
      BtnDrawLegend(&C.btn_opts[i], ui->scale, co->options[i].name);
    }
  }
}
