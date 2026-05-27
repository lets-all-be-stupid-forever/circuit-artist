#include "win_level.h"

#include "colors.h"
#include "font.h"
#include "game_registry.h"
#include "i18n.h"
#include "layout.h"
#include "sol_widget.h"
#include "stb_ds.h"
#include "ui.h"
#include "uifont.h"
#include "utils.h"
#include "wabout.h"
#include "widgets.h"
#include "win_campaign.h"
#include "win_msg.h"
#include "win_wiki.h"

#define NUM_BUTTONS 42
#define NUM_LEVELS 11
#define NUM_CAMP 10

static struct {
  LevelAPI api; /* Active level */
  GameRegistry* registry;
  LevelGroup* selected_group;
  LevelDef* selected_level;
  Layout* layout;
  char* mod_root;
  Rectangle modal;
  Rectangle group_panel;
  Rectangle buttons;
  Btn btn_option[NUM_BUTTONS];
  Btn btn_choose;
  Btn btn_close;
  Rectangle camp_title;
  Rectangle level_title;
  Rectangle r_options;
  Rectangle campicon;
  Rectangle sepv;
  Btn btn_camp;
  Btn btn_msg[8];
  Rectangle medals[NUM_BUTTONS];
  Textbox tb;
  Textbox tbcamp;
  Rectangle rcamp;
  SolWidget sol;
  void (*on_select_level)(LevelDef*);
} C = {0};

typedef struct {
  int x;
  int y;
  int rows;
  int cols;
  int w;
  int h;
  int spacex;
  int spacey;
} RectGridParam;

Rectangle grid_rect(RectGridParam param, int y, int x) {
  Rectangle r;
  r.x = param.x + (param.spacex + param.w) * x;
  r.y = param.y + (param.spacey + param.h) * y;
  r.width = param.w;
  r.height = param.h;
  return r;
}

static void update_layout() {
  layout_update_offset(C.layout);
  Layout* l = C.layout;
  C.modal = layout_rect(l, "window");
  C.rcamp = layout_rectb(l, "rcamp");

  for (int i = 0; i < NUM_LEVELS; i++) {
    C.btn_option[i].hitbox = layout_rectb(l, TextFormat("level_%d", i + 1));
    C.medals[i] = layout_rectb(l, TextFormat("medal_%d", i + 1));
  }
  C.r_options = layout_rectb(l, "r_options");

  int cols = 1;
  int bh = 17 * 2;

  for (int i = 0; i < 8; i++) {
    C.btn_msg[i].hitbox = layout_rectb(l, TextFormat("extra%d", i + 1));
  }

  C.btn_choose.hitbox = layout_rectb(l, "btn_select");
  C.btn_close.hitbox = layout_rectb(l, "btn_close");
  C.btn_camp.hitbox = layout_rectb(l, "btn_camp");
  C.camp_title = layout_rectb(l, "camp_title");
  C.sepv = layout_rect(l, "sepv");
  C.level_title = layout_rectb(l, "level_title");
  C.campicon = layout_rectb(l, "campicon");

  textbox_set_box(&C.tb, layout_rectb(l, "levelbox"));
  textbox_set_box(&C.tbcamp, layout_rectb(l, "campbox"));
  sol_widget_update_layout(&C.sol, l);
}

void win_level_set_sel(LevelDef* ldef) {
  C.selected_group = ldef->group;
  sol_widget_set_level_id(&C.sol, ldef->id);
  if (ldef == C.selected_level) return;
  C.selected_level = ldef;
  C.selected_group = ldef->group;
  textbox_set_content(&C.tb, ldef->description, ldef->sprites);
  textbox_set_content(&C.tbcamp, ldef->group->desc, NULL);
}

void win_level_init() {
  C.registry = getreg();
  sol_widget_init(&C.sol);
  textbox_init(&C.tb);
  textbox_init(&C.tbcamp);
  C.layout = easy_load_layout("level");
  C.selected_level = NULL;
}

static void select_first_level() {
  win_level_set_sel(C.selected_group->levels[0]);
}

static void select_first_available_level() {
  LevelGroup* lg = C.selected_group;
  int nl = arrlen(lg->levels);
  for (int i = 0; i < nl; i++) {
    // int j = nl - i - 1;
    int j = i;
    if (lg->levels[j]->can_choose && !lg->levels[j]->complete) {
      win_level_set_sel(lg->levels[j]);
      return;
    }
  }
  /* Selects last level if none is available */
  win_level_set_sel(lg->levels[nl - 1]);
}

void win_level_open(LevelDef* active_level,
                    void (*on_select_level)(LevelDef*)) {
  ui_winpush(WINDOW_LEVEL);
  update_layout();
  /* When window opens, it puts the active level as the selected one */
  LevelDef* sel = active_level;
  if (sel) {
    // sel = C.registry->group_order[0]->levels[0];
    win_level_set_sel(sel);
  } else {
    LevelGroup** g = getreg()->group_order;
    int ng = arrlen(g);
    for (int i = ng - 1; i >= 0; i--) {
      if (g[i]->can_choose) {
        C.selected_group = g[i];
      }
    }
    select_first_available_level();
  }
  C.on_select_level = on_select_level;
}

void win_level_accept() {
  ui_winpop();
  C.on_select_level(C.selected_level);
}

void win_level_update() {
  update_layout();
  if (btn_update(&C.btn_camp)) {
    win_campaign_open(C.selected_level->group);
    return;
  }

  bool escape = IsKeyPressed(KEY_ESCAPE);
  if (btn_update(&C.btn_close) || escape) {
    ui_winpop();
    return;
  }

  LevelDef* lvl = C.selected_level;
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

  GameRegistry* r = C.registry;
  int nc = arrlen(r->group_order);

  int nl = arrlen(C.selected_group->levels);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (i < nl) {
      if (btn_update(&C.btn_option[i])) {
        win_level_set_sel(C.selected_group->levels[i]);
      }
    }
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (i < nl) {
      LevelDef* ldef = C.selected_group->levels[i];
      /* Disabling until I fix the dependency */
      C.btn_option[i].hidden = !ldef->can_choose;
      C.btn_option[i].toggled = (C.selected_level == ldef);
    }
  }

  if (btn_update(&C.btn_choose)) {
    win_level_accept();
    return;
  }
  textbox_update(&C.tb);
  textbox_update(&C.tbcamp);
  sol_widget_update(&C.sol);
}

void win_level_draw() {
  LevelDef* lvl = C.selected_level;
  draw_win(C.modal, T.levels_title);
  textbox_draw(&C.tb);
  textbox_draw(&C.tbcamp);

  {
    Texture2D icon = C.selected_group->icon;
    Rectangle r = C.campicon;
    Rectangle source = {0, 0, icon.width, icon.height};
    DrawRectangleRec(r, (Color){0, 0, 0, 150});

    int s = 2;
    int fx = (r.x + r.width / 2) - s * source.width / 2;
    int fy = (r.y + r.height / 2) - s * source.height / 2;
    int x = r.x;
    int y = r.y;
    int w = r.width;
    int h = r.height;
    rlPushMatrix();
    rlTranslatef(fx, fy, 0);
    rlScalef(s, s, 1);
    DrawTextureRec(icon, source, (Vector2){0, 0}, CA_WHITE);
    rlPopMatrix();
    draw_frame(r);
  }

  C.btn_choose.primary = true;
  btn_draw_text(&C.btn_choose, T.levels_submit);
  btn_draw_text(&C.btn_close, T.close);
  btn_draw_text(&C.btn_camp, T.levels_change_campaign);

  {
    Color c = BLACK;
    c.a = 150;
    const char* camp_title_txt = C.selected_level->group->name;
    const char* level_title_txt = C.selected_level->name;
    DrawRectangleRec(C.camp_title, c);
    custom_label_draw_centered(C.camp_title, camp_title_txt, CA_WHITE, BLACK);
    DrawRectangleRec(C.level_title, c);
    custom_label_draw_centered(C.level_title, level_title_txt, CA_WHITE, BLACK);

    draw_frame(C.level_title);
    draw_frame(C.camp_title);
  }

  Texture2D sprites = ui_get_sprites();
  int bscale = ui_get_scale();
  // btn_draw_icon(&C.btn_msg[1], bscale, sprites, rect_book);

  int j = 0;
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

  DrawRectangleRec(C.group_panel, GRAY);
  GameRegistry* r = C.registry;

  LevelDef** levels = C.selected_group->levels;
  int nl = arrlen(levels);
  for (int i = 0; i < 11; i++) {
    Color c = BLACK;
    c.a = 150;
    DrawRectangleRec(C.btn_option[i].hitbox, c);
    DrawRectangleRec(C.medals[i], c);
  }

  int nc = arrlen(r->group_order);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (i < nl) {
      LevelDef* ldef = levels[i];
      const char* txt =
          TextFormat("%s. %s", get_roman_number(i + 1), ldef->name);
      btn_draw_text(&C.btn_option[i], txt);
      if (C.btn_option[i].hidden) {
        custom_label_draw_centered(C.btn_option[i].hitbox, txt, CA_GRAYDARK,
                                   BLACK);
      }
      if (ldef->complete) {
        draw_medal(C.medals[i], CA_WHITE);
      }
    }
  }

  draw_sepv(C.sepv);
  draw_frame(C.r_options);
  DrawRectangleRec(C.rcamp, (Color){0, 0, 0, 150});
  draw_frame(C.rcamp);
  sol_widget_draw(&C.sol);

  if (ui_get_window() == WINDOW_LEVEL) {
    sol_widget_draw_leg(&C.sol);
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
}

