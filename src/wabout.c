#include "wabout.h"

#include "common.h"
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
      17 * 2,
  };
  textbox_set_box(&C.tb, box2);
}

void easy_about_open() {
  const char* content =
      "\n!img:0\n\n" CA_VERSION
      "\n"
      "A game by `lets_all_be_stupid_forever`.\n"
      "circuitartistgame@gmail.com\n"
      "\n"
      "Available on "
      "Steam.\nhttps://store.steampowered.com/app/3139580/Circuit_Artist/"
      "\n\n"
      "Join us on Discord!\n\n"
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
      "Lua 5.4\n"
      "https://www.lua.org/\n\n"
      "`Data Structures in C (array/hashmaps)`\nSTB Library\n"
      "https://github.com/nothings/stb\n\n"
      "`Lua Serialization`\nmsgpack-c\n"
      "https://github.com/msgpack/msgpack-c\n\n"
      "`JSON parsing from C`\nJSON-C - A JSON implementation in C\n"
      "https://github.com/json-c/json-c\n\n"
      "`Trailer Music`\n"
      "Music by Kevin MacLeod\n"
      "https://freepd.com/electronic.php\n\n"
      "`Classes in lua`\nclassic.lua by rxi\n"
      "https://github.com/rxi/classic\n\n"
      "`JSON in lua`\njson.lua by rxi\n"
      "https://github.com/rxi/json.lua";
  about_open("About", content, C.about_sprites);
}

void easy_blinking_open() {
  const char* content =
      "\n`Photosensitivity Warning`\n\n"
      " A small percentage of people may experience seizures when exposed to "
      "certain visual effects, including `flashing lights`.\n\n"
      "If you experience `discomfort, dizziness, or disorientation`, stop "
      "playing immediately.\n";
  about_open("Photosensitivity Warning", content, NULL);
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
  btn_draw_text(&C.bClose, ui_get_scale(), "CLOSE");
}
