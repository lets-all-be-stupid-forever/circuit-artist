#include "levelsidebar.h"

#include "layout.h"
#include "sol_widget.h"
#include "stb_ds.h"
#include "stdio.h"
#include "utils.h"
#include "win_main.h"
#include "win_msg.h"
#include "win_wiki.h"

static struct {
  LevelDef* ldef;
  Layout* layout;
  Textbox tb;
  Rectangle level_title;
  Rectangle modal;
  Btn btn_msg[8];
  Btn btn_close;
  Rectangle region;
  SolWidget sol;
} C = {0};

static void update_layout(Rectangle region) {
  Layout* l = C.layout;
  layout_update_offset_region(l, region);
  C.modal = layout_rect(l, "window");
  int dy = region.height - C.modal.height;
  C.modal.height += dy;
  C.region = region;
  Rectangle lb = layout_rectb(l, "levelbox");
  /* The textbox is stretched*/
  lb.height += dy;
  C.level_title = layout_rectb(l, "level_title");
  textbox_set_box(&C.tb, lb);
  l->off.y += dy;
  C.btn_close.hitbox = layout_rectb(l, "btn_close");
  for (int i = 0; i < 8; i++) {
    C.btn_msg[i].hitbox = layout_rectb(l, TextFormat("extra%d", i + 1));
  }
  sol_widget_update_layout(&C.sol, l);
  l->off.y -= dy;
}

void level_sidebar_init() {
  C.layout = easy_load_layout("levelsidebar");
  layout_scale(C.layout);
  C.layout->align_top = true;
  // C.layout->align_left = true;

  sol_widget_init(&C.sol);
  textbox_init(&C.tb);
}

void level_sidebar_set_lvl(LevelDef* ldef) {
  C.ldef = ldef;
  textbox_set_content(&C.tb, ldef->description, ldef->sprites);
  sol_widget_set_level_id(&C.sol, ldef->id);
}

void level_sidebar_update(Rectangle region) {
  update_layout(region);
  textbox_update(&C.tb);
  if (btn_update(&C.btn_close)) {
    win_main_toggle_sidebar();
  }
  sol_widget_update(&C.sol);
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

static void draw_level_title() {
  const char* level_title_txt = C.ldef->name;
  DrawRectangleRec(C.level_title, (Color){0, 0, 0, 150});
  custom_label_draw_centered(C.level_title, level_title_txt, CA_WHITE, BLACK);
  draw_frame(C.level_title);
}

void level_sidebar_draw() {
  draw_default_tiled_frame(C.region);
  draw_level_title();
  textbox_draw(&C.tb);
  draw_level_extra_items();
  btn_draw_icon(&C.btn_close, rect_close);
  sol_widget_draw(&C.sol);
}

void level_sidebar_draw_legend() {
  sol_widget_draw_leg(&C.sol);
  LevelDef* lvl = C.ldef;
  int nextra = arrlen(lvl->extra_content);
  for (int i = 0; i < nextra; i++) {
    LevelDefExtraItem* item = &lvl->extra_content[i];
    if (item->text || item->tex.width > 0) {
      btn_draw_legend(&C.btn_msg[i], lvl->extra_content[i].title);
    }
    if (item->wiki) {
      TutorialItem* wiki_item = find_wiki_from_id(item->wiki);
      if (wiki_item) {
        btn_draw_legend(&C.btn_msg[i], wiki_item->name);
      }
    }
  }
}
