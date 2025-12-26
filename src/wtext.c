#include "wtext.h"

#include "colors.h"
#include "font.h"
#include "rlgl.h"
#include "string.h"
#include "ui.h"
#include "utils.h"
#include "wmain.h"

#define MAX_TEXT_SIZE 1000

static struct {
  int tlen;
  char txt[MAX_TEXT_SIZE];
  float alive;
  void* ctx;
  void (*on_accept)(void* ctx, const char* txt);
} C = {0};

// void text_modal_open() {
void text_modal_open(void (*on_accept)(void* ctx, const char* txt), void* ctx,
                     const char* initial_text) {
  ui_winpush(WINDOW_TEXT);

  C.on_accept = on_accept;
  C.ctx = ctx;
  C.alive = 0;
  if (initial_text) {
    strncpy(C.txt, initial_text, sizeof(C.txt) - 1);
    C.tlen = strlen(C.txt);
  } else {
    C.tlen = 0;
    C.txt[0] = '\0';
  }
}

void text_modal_update() {
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
  escape = escape || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
           IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

  if (escape) {
    C.tlen = 0;
    ui_winpop();
    return;
  }

  if (IsKeyPressed(KEY_ENTER)) {
    C.alive = 0;
    int tlen = C.tlen;
    C.on_accept(C.ctx, C.txt);
    ui_winpop();
    return;
  }
}

void text_modal_draw() {
  Color bg = BLACK;
  bg.a = 150;
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  DrawRectangle(0, 0, sw, sh, bg);
  Color c1 = get_lut_color(COLOR_BG0);
  int cursor_type = ((int)(3 * C.alive)) % 2;
  int s = ui_get_scale();
  int hh = 50;
  int y0 = (sh - hh * s) / 2;
  rlPushMatrix();

  draw_bg((Rectangle){0, y0, 2 * sw, 2 * hh});

  rlTranslatef(0, y0, 0);
  rlScalef(s, s, 1);
  // DrawRectangle(0, 0, sw, hh, c1);

  const char* cap = "Insert Text:";
  int tx = get_rendered_text_size(cap).x;

  rlTranslatef(sw / s / 2, 10, 0);

  rlTranslatef(-tx / 2, 0, 0);
  font_draw_texture(cap, 1, 1, BLACK);
  font_draw_texture(cap, 0, 0, WHITE);
  rlTranslatef(tx / 2, 0, 0);

  rlTranslatef(0, 20, 0);

  tx = get_rendered_text_size(C.txt).x;
  rlTranslatef(-tx / 2, 0, 0);
  font_draw_texture(C.txt, 1, 1, BLACK);
  font_draw_texture(C.txt, 0, 0, WHITE);
  if (cursor_type == 0) {
    rlTranslatef(tx + 1, 0, 0);
    DrawRectangle(0, 0, 1 + 1, 7 + 1, BLACK);
    DrawRectangle(0, 0, 1, 7, WHITE);
  }
  rlPopMatrix();

  Rectangle inner_content = (Rectangle){-12, y0, GetScreenWidth() + 24, hh * s};
  draw_default_tiled_frame(inner_content);
}
