#include "w_text.h"

#include "colors.h"
#include "font.h"
#include "rlgl.h"
#include "tiling.h"
#include "ui.h"
#include "w_main.h"

#define MAX_TEXT_SIZE 1000

static struct {
  int tlen;
  char txt[MAX_TEXT_SIZE];
  float alive;
} C = {0};

void TextModalOpen(Ui* ui)
{
  ui->window = WINDOW_TEXT;
  C.alive = 0;
  C.tlen = 0;
  C.txt[0] = '\0';
}

void TextModalUpdate(Ui* ui)
{
  float dt = GetFrameTime();
  C.alive += dt;
  int key = GetCharPressed();
  int max_input_chars = MAX_TEXT_SIZE;
  while (key > 0) {
    if ((key >= 32) && (key <= 125) && (C.tlen < max_input_chars)) {
      C.txt[C.tlen] = (char)key;
      C.txt[++C.tlen] = '\0';  // Add null terminator at the end of the string.
    }
    key = GetCharPressed();  // Check next character in the queue
    C.alive = 0;
  }

  if (IsKeyPressed(KEY_BACKSPACE)) {
    C.tlen--;
    if (C.tlen < 0) C.tlen = 0;
    C.txt[C.tlen] = '\0';
    C.alive = 0;
  }

  bool escape = IsKeyPressed(KEY_ESCAPE);
  escape = escape || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

  if (escape) {
    C.tlen = 0;
    ui->window = WINDOW_MAIN;
    return;
  }

  if (IsKeyPressed(KEY_ENTER)) {
    C.alive = 0;
    int tlen = C.tlen;
    if (tlen > 0) {
      MainPasteText(C.txt);
    }
    ui->window = WINDOW_MAIN;
    return;
  }
}

void TextModalDraw(Ui* ui)
{
  Color bg = BLACK;
  bg.a = 150;
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  DrawRectangle(0, 0, sw, sh, bg);
  Color c1 = GetLutColor(COLOR_BG0);
  int cursor_type = ((int)(3 * C.alive)) % 2;
  int s = ui->scale;
  int hh = 50;
  int y0 = (sh - hh * s) / 2;
  rlPushMatrix();

  rlTranslatef(0, y0, 0);
  rlScalef(s, s, 1);
  DrawRectangle(0, 0, sw, hh, c1);

  const char* cap = "Insert Text:";
  int tx = GetRenderedTextSize(cap).x;

  rlTranslatef(sw / s / 2, 10, 0);

  rlTranslatef(-tx / 2, 0, 0);
  FontDrawTexture(cap, 1, 1, BLACK);
  FontDrawTexture(cap, 0, 0, WHITE);
  rlTranslatef(tx / 2, 0, 0);

  rlTranslatef(0, 20, 0);

  tx = GetRenderedTextSize(C.txt).x;
  rlTranslatef(-tx / 2, 0, 0);
  FontDrawTexture(C.txt, 1, 1, BLACK);
  FontDrawTexture(C.txt, 0, 0, WHITE);
  if (cursor_type == 0) {
    rlTranslatef(tx + 1, 0, 0);
    DrawRectangle(0, 0, 1 + 1, 7 + 1, BLACK);
    DrawRectangle(0, 0, 1, 7, WHITE);
  }
  rlPopMatrix();

  Rectangle inner_content = (Rectangle){
      -12,
      y0,
      GetScreenWidth() + 24,
      hh * s};
  DrawDefaultTiledFrame(ui, inner_content);
}

