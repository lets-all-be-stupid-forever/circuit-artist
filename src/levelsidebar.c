#include "levelsidebar.h"

#include "stb_ds.h"
#include "utils.h"
#include "win_main.h"
#include "win_msg.h"
#include "win_wiki.h"

void level_sidebar_update_layout(LevelSidebarWidget* w, Rectangle region) {
  Layout* l = w->layout;
  w->region = region;
  layout_update_offset_region(l, region);
  w->level_title = layout_rectb(l, "level_title");
  for (int i = 0; i < 8; i++) {
    w->btn_msg[i].hitbox = layout_rectb(l, TextFormat("extra%d", i + 1));
  }
  textbox_set_box(&w->tb, layout_rectb(l, "levelbox"));
  w->btn_close.hitbox = layout_rectb(l, "btn_close");
}

void level_sidebar_init(LevelSidebarWidget* w) {
  textbox_init(&w->tb);
  w->layout = easy_load_layout("levelsidebar");
  level_sidebar_set_level(w, NULL);
}

void level_sidebar_set_level(LevelSidebarWidget* w, LevelDef* ldef) {
  w->ldef = ldef;
  if (ldef) {
    textbox_set_content(&w->tb, ldef->description, ldef->sprites);
  } else {
    textbox_set_content(&w->tb, "", NULL);
  }
}

void level_sidebar_update(LevelSidebarWidget* w) {
  LevelDef* lvl = w->ldef;
  if (lvl) {
    int nextra = arrlen(lvl->extra_content);
    for (int i = 0; i < nextra; i++) {
      if (btn_update(&w->btn_msg[i])) {
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
  textbox_update(&w->tb);
  if (btn_update(&w->btn_close)) {
    w->on_sidebar_close();
  }
}

static void draw_cap(Rectangle r, const char* txt) {
  DrawRectangleRec(r, (Color){0, 0, 0, 150});
  custom_label_draw_centered(r, txt, CA_WHITE, BLACK);
  draw_frame(r);
}

static void draw_level_title(LevelSidebarWidget* w) {}

static void draw_level_extra_items(LevelSidebarWidget* w) {
  LevelDef* lvl = w->ldef;
  if (!lvl) return;
  int j = 0;
  int nextra = arrlen(lvl->extra_content);
  for (int i = 0; i < nextra; i++) {
    LevelDefExtraItem* item = &lvl->extra_content[i];
    if (item->text) {
      btn_draw_icon(&w->btn_msg[j++], rect_bulbon);
    }
    if (item->wiki) {
      btn_draw_icon(&w->btn_msg[j++], rect_book);
    }
    if (item->tex.width > 0) {
      btn_draw_icon(&w->btn_msg[j++], rect_img);
    }
  }
  for (int i = j; i < 8; i++) {
    Color c = BLACK;
    c.a = 150;
    DrawRectangleRec(w->btn_msg[j++].hitbox, c);
  }
}

void level_sidebar_draw(LevelSidebarWidget* w) {
  // draw_frame(w->region);
  draw_default_tiled_frame(w->region);
  draw_cap(w->level_title, w->ldef ? w->ldef->name : "");
  textbox_draw(&w->tb);
  draw_level_extra_items(w);
  btn_draw_icon(&w->btn_close, rect_close);
}

void level_sidebar_draw_leg(LevelSidebarWidget* w) {
  // TODO
}
