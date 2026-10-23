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
#include "win_msg.h"
#include "win_wiki.h"

#define NUM_BUTTONS 42
#define NUM_LEVELS 11
#define NUM_CAMP 6

static struct {
  LevelAPI api; /* Active level */
  GameRegistry* registry;
  LevelGroup** camps;
  int nc; /* num of campaigns */
  LevelDef* selected_level;
  Layout* layout;
  Rectangle roadmap_container;
  Rectangle campbg;
  char* mod_root;
  Rectangle modal;
  Rectangle buttons;
  Btn btn_option[NUM_BUTTONS];
  Btn btn_choose;
  Btn btn_close;
  Rectangle camp_title;
  Rectangle level_title;
  Rectangle roadmap_title;
  Rectangle r_options;
  Rectangle sepv1;
  Rectangle sepv2;
  Btn btn_camp[NUM_CAMP];
  Btn btn_msg[8];
  Rectangle medals[NUM_BUTTONS];
  Textbox tb;
  Textbox tbcamp;
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
  C.campbg = layout_rectb(l, "campbg");

  for (int i = 0; i < NUM_LEVELS; i++) {
    C.btn_option[i].hitbox = layout_rectb(l, TextFormat("level_%d", i + 1));
    C.medals[i] = layout_rectb(l, TextFormat("medal_%d", i + 1));
  }
  C.r_options = layout_rectb(l, "r_options");

  for (int i = 0; i < NUM_CAMP; i++) {
    C.btn_camp[i].hitbox = layout_rectb(l, TextFormat("camp%d", i + 1));
  }

  for (int i = 0; i < 8; i++) {
    C.btn_msg[i].hitbox = layout_rectb(l, TextFormat("extra%d", i + 1));
  }

  C.roadmap_container = layout_rectb(l, "roadmap_container");
  C.btn_choose.hitbox = layout_rectb(l, "btn_select");
  C.btn_close.hitbox = layout_rectb(l, "btn_close");
  C.camp_title = layout_rectb(l, "camp_title");
  C.sepv1 = layout_rect(l, "sepv1");
  C.sepv2 = layout_rect(l, "sepv2");
  C.level_title = layout_rectb(l, "level_title");
  C.roadmap_title = layout_rectb(l, "roadmap_title");

  textbox_set_box(&C.tb, layout_rectb(l, "levelbox"));
  textbox_set_box(&C.tbcamp, layout_rectb(l, "campbox"));
  sol_widget_update_layout(&C.sol, l);
}

void win_level_set_sel(LevelDef* ldef) {
  sol_widget_set_level_id(&C.sol, ldef->id);
  if (ldef == C.selected_level) return;
  C.selected_level = ldef;
  textbox_set_content(&C.tb, ldef->description, ldef->sprites);
  textbox_set_content(&C.tbcamp, ldef->group->desc, NULL);
}

void win_level_init() {
  C.registry = getreg();
  C.camps = getreg()->group_order;
  C.nc = arrlen(C.camps);
  sol_widget_init(&C.sol);
  textbox_init(&C.tb);
  textbox_init(&C.tbcamp);
  C.layout = easy_load_layout("levelv2");
  C.selected_level = NULL;
}

static void select_first_level() {
  win_level_set_sel(C.selected_level->group->levels[0]);
}

static void select_first_available_level(LevelGroup* lg) {
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
  if (sel->is_campaign) {
    // sel = C.registry->group_order[0]->levels[0];
    win_level_set_sel(sel);
  } else {
    LevelGroup** g = getreg()->group_order;
    LevelGroup* selected = NULL;
    int ng = arrlen(g);
    for (int i = ng - 1; i >= 0; i--) {
      if (g[i]->can_choose) {
        selected = g[i];
      }
    }
    select_first_available_level(selected);
  }
  C.on_select_level = on_select_level;
}

void win_level_accept() {
  ui_winpop();
  C.on_select_level(C.selected_level);
}

static LevelDef* find_best_level(LevelGroup* g) {
  int nl = arrlen(g->levels);
  for (int i = 0; i < nl; i++) {
    // int j = nl - i - 1;
    int j = i;
    if (g->levels[j]->can_choose && !g->levels[j]->complete) {
      return g->levels[j];
    }
  }
  /* Selects last level if all are complete */
  return g->levels[nl - 1];
}

void win_level_update() {
  update_layout();
  C.btn_choose.primary = true;

  for (int i = 0; i < C.nc; i++) {
    if (btn_update(&C.btn_camp[i])) {
      LevelDef* ldef = find_best_level(C.camps[i]);
      win_level_set_sel(ldef);
    }
  }
  for (int i = 0; i < C.nc; i++) {
    C.btn_camp[i].disabled = !C.camps[i]->can_choose;
    C.btn_camp[i].toggled = C.camps[i] == C.selected_level->group;
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

  int nl = arrlen(C.selected_level->group->levels);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (i < nl) {
      if (btn_update(&C.btn_option[i])) {
        win_level_set_sel(C.selected_level->group->levels[i]);
      }
    }
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (i < nl) {
      LevelDef* ldef = C.selected_level->group->levels[i];
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

static void draw_campaign_level_list() {
  GameRegistry* r = C.registry;
  LevelDef** levels = C.selected_level->group->levels;
  int nl = arrlen(levels);
  // draw_frame(C.r_options);
  for (int i = 0; i < NUM_LEVELS; i++) {
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
      C.btn_option[i].primary = !ldef->complete;
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
}

static void draw_campaign_title() {
  const char* camp_title_txt = C.selected_level->group->name;
  DrawRectangleRec(C.camp_title, (Color){0, 0, 0, 150});
  custom_label_draw_centered(C.camp_title, camp_title_txt, CA_WHITE, BLACK);
  // draw_frame(C.camp_title);
}

static void draw_campaign_side() {
  draw_campaign_title();
  textbox_draw(&C.tbcamp);
  draw_campaign_level_list();
  sol_widget_draw(&C.sol);
}

static void draw_level_title() {
  const char* level_title_txt = C.selected_level->name;
  DrawRectangleRec(C.level_title, (Color){0, 0, 0, 150});
  custom_label_draw_centered(C.level_title, level_title_txt, CA_WHITE, BLACK);
  // draw_frame(C.level_title);
}

static void draw_level_extra_items() {
  int j = 0;
  LevelDef* lvl = C.selected_level;
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

static void draw_level_side() {
  draw_level_title();
  textbox_draw(&C.tb);
  draw_level_extra_items();
  btn_draw_text(&C.btn_choose, T.levels_submit);
  btn_draw_text(&C.btn_close, T.close);
}

static void draw_roadmap_side() {
  for (int i = 0; i < NUM_CAMP; i++) {
    Color c = BLACK;
    c.a = 150;
    // DrawRectangleRec(C.btn_camp[i].hitbox, c);
  }

  Color c = BLACK;
  c.a = 150;
  DrawRectangleRec(C.campbg, c);

#if 0
  for (int i = 0; i < C.nc; i++) {
    LevelGroup* g = C.camps[i];
    int x = C.btn_camp[i].hitbox.x;
    int y = C.btn_camp[i].hitbox.y;
    int w = C.btn_camp[i].hitbox.width;
    int h = C.btn_camp[i].hitbox.height;
    Rectangle rmed = (Rectangle){x, y - 34, w, 34};
    Color c = BLACK;
    c.a = 150;
    DrawRectangleRec(rmed, c);
    if (g->complete) {
      draw_medal(rmed, CA_WHITE);
    }
  }
#endif

  for (int i = 0; i < C.nc; i++) {
    LevelGroup* g = C.camps[i];
    Texture2D texture = g->icon;
    Rectangle rect = {0, 0, g->icon.width, g->icon.height};
    Rectangle source = rect;
    if (g->can_choose) {
      C.btn_camp[i].primary = !g->complete;
      btn_draw_icon2(&C.btn_camp[i], ui_get_scale(), texture,
                     source);  // c->name);

#if 0
      Rectangle r = C.btn_camp[i].hitbox;
      Color k = CA_WHITE;
      if (C.btn_camp[i].toggled) {
        DrawRectangleRec(r, CA_ORANGE);
        k = BLACK;
      } else {
        // Color c = BLACK;
        // c.a = 150;
        // DrawRectangleRec(r, c);
        k = CA_WHITE;
      }

      int s = ui_get_scale();
      int fx = r.x + (r.width - s * rect.width) / 2;
      int fy = r.y + (r.height - s * rect.height) / 2;
      rlPushMatrix();
      rlTranslatef(fx, fy, 0);
      rlScalef(s, s, 1);
      DrawTextureRec(texture, source, (Vector2){0, 0}, k);
      rlPopMatrix();
#endif

    } else {
      Rectangle r = C.btn_camp[i].hitbox;
      Color k = CA_GRAYDARK;

      int s = ui_get_scale();
      int fx = r.x + (r.width - s * rect.width) / 2;
      int fy = r.y + (r.height - s * rect.height) / 2;
      rlPushMatrix();
      rlTranslatef(fx, fy, 0);
      rlScalef(s, s, 1);
      DrawTextureRec(texture, source, (Vector2){0, 0}, k);
      rlPopMatrix();
    }
  }
}

void win_level_draw() {
  LevelDef* lvl = C.selected_level;
  draw_win(C.modal, T.levels_title);
  draw_campaign_side();
  draw_level_side();
  draw_roadmap_side();
  // draw_sepv(C.sepv1);
  // draw_sepv(C.sepv2);
  if (ui_get_window() == WINDOW_LEVEL) {
    sol_widget_draw_leg(&C.sol);
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
}

