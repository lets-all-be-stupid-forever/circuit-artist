#include "levelsidebar.h"

#include "layout.h"
#include "stb_ds.h"
#include "utils.h"
#include "win_msg.h"
#include "win_wiki.h"

static struct {
  LevelDef* ldef;
  Layout* layout;
  Textbox tb;
  Rectangle capt;
  Rectangle modal;
  Btn btn_msg[8];
  Btn btn_close;
  Rectangle region;
} C = {0};

#define LAYBTN(b, n) (b).hitbox = layout_rectb(l, #n)

static void update_layout(Rectangle region) {
  Layout* l = C.layout;
  layout_update_offset_region(l, region);
  C.modal = layout_rectb(l, "window");
  C.region = region;
  C.btn_close.hitbox = layout_rectb(l, "btn_close");
  LAYBTN(C.btn_close, btn_close);
  Rectangle r = region;
  for (int i = 0; i < 8; i++) {
    Rectangle rex = {
        r.x + 4 + 36 * i,
        r.y,
        38,
        38,
    };
    C.btn_msg[i].hitbox = rex;  // layout_rectb(l, TextFormat("extra%d", i +
                                // 1));
  }
  C.capt = layout_rectb(l, "level_title");
  Rectangle rtb = {
      r.x + 4,
      r.y + 50,
      r.width,
      r.height,
  };
  // textbox_set_box(&C.tb, layout_rectb(l, "levelbox"));
  textbox_set_box(&C.tb, rtb);
}

void level_sidebar_init() {
  C.layout = easy_load_layout("levelsidebar");

  textbox_init(&C.tb);
}

void level_sidebar_set_lvl(LevelDef* ldef) {
  C.ldef = ldef;
  textbox_set_content(&C.tb, ldef->description, ldef->sprites);
}

void level_sidebar_update(Rectangle region) {
  update_layout(region);
  textbox_update(&C.tb);
  LevelDef* lvl = C.ldef;
  int nextra = arrlen(lvl->extra_content);
  for (int i = 0; i < nextra; i++) {
    if (btn_update(&C.btn_msg[i])) {
      LevelDefExtraItem* item = &lvl->extra_content[i];
      if (item->text) {
        win_msg_open_text(item->text, NULL);
      }
      if (item->wiki) {
        win_wiki_open_on_item(item->wiki);
      }
      if (item->tex.width > 0) {
        win_msg_open_tex(item->tex, item->scale);
      }
      return;
    }
  }
}

static void draw_level_extra_items() {
  int j = 0;
  LevelDef* lvl = C.ldef;
  int nextra = arrlen(lvl->extra_content);
  for (int i = 0; i < nextra; i++) {
    LevelDefExtraItem* item = &lvl->extra_content[i];
    if (item->text) {
      btn_draw_icon(&C.btn_msg[j++], rect_bulbon);
    }
    if (item->wiki) {
      btn_draw_icon(&C.btn_msg[j++], rect_book);
    }
    if (item->tex.width > 0) {
      btn_draw_icon(&C.btn_msg[j++], rect_img);
    }
  }
  for (int i = j; i < 8; i++) {
    Color c = BLACK;
    c.a = 150;
    DrawRectangleRec(C.btn_msg[j++].hitbox, c);
  }
}

void level_sidebar_draw() {
  // draw_win(C.modal, "level");
  textbox_draw(&C.tb);
  draw_level_extra_items();
}

