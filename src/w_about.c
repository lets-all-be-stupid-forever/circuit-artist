#include "w_about.h"

#include "font.h"
#include "rlgl.h"
#include "tiling.h"
#include "ui.h"
#include "version.h"
#include "widgets.h"

static struct {
  bool inited;
  Textbox tb;
  Rectangle textbox_wrap;
  Rectangle title;
  Rectangle modal;
  Sprite sprites[2];
  bool closed;
  Btn btn_close;
} C = {0};

static void AboutUpdateLayout() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int bw = 12 * 35 * 2;
  int s = 2;
  int total_w = bw + 2 * 2;
  int x = (sw - total_w) / 2;
  int bh = 9 * 35 * s;
#ifdef WEB
  // Making it smaller for the web version
  bh = 4 * 35 * 2;
#endif
  int total_h = (bh + 35 * 2 + 35 * 2);
  int pad = 10 * s;
  while (total_h + 2 * pad + 10 > sh) {
    total_h -= 35 * s;
    bh -= 35 * s;
  }
  int y = (sh - total_h) / 2;
  int yy = y + 35 * 2;
  C.title = (Rectangle){x, y, total_w, 35 * 2};
  C.modal = (Rectangle){x - pad, y - pad, total_w + 2 * pad, total_h + 2 * pad};
  Rectangle box = {x + 2 * s, yy, bw, bh};
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
}

void AboutOpen(Ui* ui) {
  ui->window = WINDOW_ABOUT;
  const char* about_page =
      "\n!img:0\n\n" CA_VERSION
      "\n"
      "A game by `lets_all_be_stupid_forever`.\n"
      "circuitartistgame@gmail.com\n"
      "\n"
      "Available on "
      "Steam.\nhttps://store.steampowered.com/app/3139580/Circuit_Artist/\n\n"
      "\n`Credits:`"
      "\n!hl"
      "\n"
      "\n`Game Framework`"
      "\nRaylib + C"
      "\nraylib.com"
      "\n\n"
      "`Font`\n"
      "m5x7 and m3x6 by Daniel Linssen.\n"
      "https://managore.itch.io/m5x7\n"
      "\n"
      "`File Dialog`\n"
      "Native File Dialog by Michael Labbe\n"
      "https://github.com/mlabbe/nativefiledialog\n\n"
      "`Copy-paste`\n"
      "Clip library by David Capello / Aseprite.\n"
      "https://github.com/aseprite/clip\n\n"
      "`Scripts`\n"
      "Luajit\n"
      "https://luajit.org/\n\n"
      "`Trailer Music`\n"
      "Music by Kevin MacLeod\n"
      "https://freepd.com/electronic.php\n\n"
      "`Classes in lua`\nclassic.lua by rxi\n"
      "https://github.com/rxi/classic\n\n"
      "`JSON in lua`\njson.lua by rxi\n"
      "https://github.com/rxi/json.lua";
  C.closed = false;
  if (!C.inited) {
    C.inited = true;
    TextboxLoad(&C.tb);
    C.sprites[0] = (Sprite){
        .tex = ui->sprites,
        .region = rect_logo,
    };
    TextboxSetContent(&C.tb, about_page, C.sprites);
  }
}

void AboutUpdate(Ui* ui) {
  AboutUpdateLayout();
  if (BtnUpdate(&C.btn_close, ui) || IsKeyPressed(KEY_ESCAPE)) {
    ui->window = WINDOW_MAIN;
    return;
  }
  TextboxUpdate(&C.tb, ui);
}

void AboutDraw(Ui* ui) {
  int sh = GetScreenHeight();
  int sw = GetScreenWidth();
  Color bg = BLACK;
  bg.a = 150;
  DrawRectangle(0, 0, sw, sh, bg);
  const char* title = "ABOUT";
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
  BtnDrawText(&C.btn_close, ui->scale, "CLOSE");
  DrawDefaultTiledFrame(ui, C.modal);
}
