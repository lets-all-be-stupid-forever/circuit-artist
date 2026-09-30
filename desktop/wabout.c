#include "wabout.h"

#include "common.h"
#include "i18n.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

static struct {
  Textbox tb;
  char* title;
  char* content;
  Rectangle tbWrap;
  Rectangle modal;
  sprite_t* sprites;
  sprite_t about_sprites[5];
  Btn bClose;
} C = {0};

static void about_update_layout() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int bw = 12 * 35 * 2;
  int s = 2;
  int total_w = bw + 2 * 2;
  int x = (sw - total_w) / 2;
  int bh = 9 * 35 * s;
  int total_h = (bh + 35 * 1 + 35 * 1);
  int pad = 10 * s;
  while (total_h + 2 * pad + 10 > sh) {
    total_h -= 35 * s;
    bh -= 35 * s;
  }
  int y = (sh - total_h) / 2;
  int yy = y + 35 * 1;
  C.modal = (Rectangle){x - pad, y - pad, total_w + 2 * pad, total_h + 2 * pad};
  Rectangle box = {x + 2 * s, yy, bw, bh - 4 * s};
  C.tbWrap = box;
  Rectangle box2 = box;
  int bsize = 4 * 35 * s;
  C.bClose.hitbox = (Rectangle){
      box.x + box.width - bsize,
      box.y + box.height + 8 * 2,
      bsize,
      19 * 2,
  };
  textbox_set_box(&C.tb, box2);
}

void easy_about_open() {
  about_open(T.about_title, T.about_content, C.about_sprites);
}

void easy_blinking_open() {
  about_open(T.about_photosensitivity_title, T.about_photosensitivity_text,
             NULL);
}

void about_init() {
  textbox_init(&C.tb);
  about_update_layout();
  C.about_sprites[0] = (sprite_t){
      .tex = ui_get_sprites(),
      .region = rect_logo,
  };
}

void about_open(const char* title, const char* content, sprite_t* sprites) {
  ui_winpush(WINDOW_ABOUT);
  free(C.title);
  C.title = clone_string(title);
  textbox_set_content(&C.tb, content, sprites);
}

void about_update() {
  about_update_layout();
  if (btn_update(&C.bClose) || IsKeyPressed(KEY_ESCAPE)) {
    ui_winpop();
    return;
  }
  textbox_update(&C.tb);
}

void about_draw() {
  draw_win(C.modal, C.title);
  textbox_draw(&C.tb);
  btn_draw_text(&C.bClose, T.close);
}
