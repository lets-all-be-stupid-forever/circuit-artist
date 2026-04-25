#include "win_level.h"

#include "colors.h"
#include "font.h"
#include "game_registry.h"
#include "layout.h"
#include "stb_ds.h"
#include "ui.h"
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
  Label campaign_title;
  Btn btn_choose;
  Btn btn_close;
  Btn btn_campaign[10];
  Rectangle med_camp[10];
  Btn btn_msg[8];
  Rectangle medals[NUM_BUTTONS];
  Label level_title;
  Textbox tb;
  Textbox tb_grp;
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

static void update_campaign_title() {
  label_set_text(&C.campaign_title, C.selected_group->name);
}
static void update_layout() {
  layout_update_offset(C.layout);
  Layout* l = C.layout;
  C.modal = layout_rect(l, "window");

  C.level_title.hitbox = layout_rect(l, "leveltitle");

  for (int i = 0; i < NUM_LEVELS; i++) {
    C.btn_option[i].hitbox = layout_rect(l, TextFormat("level_%d", i + 1));
    C.medals[i] = layout_rect(l, TextFormat("medal_%d", i + 1));
  }

  int cols = 1;
  int bh = 17 * 2;

  for (int i = 0; i < 8; i++) {
    C.btn_msg[i].hitbox = layout_rect(l, TextFormat("extra%d", i + 1));
  }

  C.btn_choose.hitbox = layout_rect(l, "btn_select");
  C.btn_close.hitbox = layout_rect(l, "btn_close");
  // C.btn_campaign2.hitbox = roff(off, WIN_LEVEL_campaign);
  C.campaign_title.hitbox = layout_rect(l, "camptitle");

  for (int i = 0; i < NUM_CAMP; i++) {
    C.btn_campaign[i].hitbox = layout_rect(l, TextFormat("camp%d", i + 1));
    C.med_camp[i] = layout_rect(l, TextFormat("campmed%d", i + 1));
  }

  textbox_set_box(&C.tb, layout_rect(l, "levelbox"));
  textbox_set_box(&C.tb_grp, layout_rect(l, "campbox"));
}

void win_level_set_sel(LevelDef* ldef) {
  C.selected_group = ldef->group;
  if (ldef == C.selected_level) return;
  C.selected_level = ldef;
  C.selected_group = ldef->group;
  update_campaign_title();
  textbox_set_content(&C.tb, ldef->description, ldef->sprites);
  textbox_set_content(&C.tb_grp, ldef->group->desc,
                      NULL); /* no sprites for group*/
  label_set_text(&C.level_title, ldef->name);
}

void win_level_init(GameRegistry* r) {
  C.registry = r;
  textbox_init(&C.tb);
  textbox_init(&C.tb_grp);
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
    // lg->levels[j]->can_choose &&
    if (!lg->levels[j]->complete) {
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
  if (!sel) {
    sel = C.registry->group_order[0]->levels[0];
  }
  win_level_set_sel(sel);
  C.on_select_level = on_select_level;
}

void win_level_set_campaign(int icampaign) {
  if (icampaign == -1) {
    return;
  }
  LevelGroup* new_group = C.registry->group_order[icampaign];
  if (C.selected_group != new_group) {
    C.selected_group = new_group;
    update_campaign_title();
    if (icampaign > 0) {
      select_first_available_level();
    } else {
      select_first_level();
    }
  }
}

static void update_campaign_buttons() {
  GameRegistry* r = C.registry;
  int nc = arrlen(r->group_order);
  Vector2 mouse = GetMousePosition();
  for (int i = 0; i < nc; i++) {
    if (btn_update(&C.btn_campaign[i])) {
      win_level_set_campaign(i);
    }
#if 0
    Rectangle h = C.btn_campaign[i].hitbox;
    bool disabled = !r->group_order[i]->can_choose;
    bool hover = rect_hover(h, mouse) && ui_get_hit_count() == 0;
    C.btn_campaign[i].hover = hover;
    if (hover) {
      ui_inc_hit_count();
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !disabled) {
        win_level_set_campaign(i);
        return;
      }
    }
#endif
  }
  for (int i = 0; i < nc; i++) {
    C.btn_campaign[i].toggled = C.registry->group_order[i] == C.selected_group;
  }
}

void win_level_update() {
  update_layout();
  update_campaign_buttons();

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
      C.btn_option[i].disabled = false;  // ! ldef->can_choose;
      C.btn_option[i].toggled = (C.selected_level == ldef);
    }
  }

  if (btn_update(&C.btn_choose)) {
    C.on_select_level(C.selected_level);
    ui_winpop();
    return;
  }
  textbox_update(&C.tb);
  textbox_update(&C.tb_grp);
}

static void draw_medal(Rectangle r) {
  rlPushMatrix();
  Texture sprites = ui_get_sprites();
  int mx = r.x;
  int my = r.y;
  int mw = r.width;
  int mh = r.height;
  int x = mx + (mw - 26) / 2;
  int y = my + (mh - 26) / 2;
  rlTranslatef(x, y, 0);
  rlScalef(2, 2, 1);
  Rectangle dst = {0, 0, 13, 13};
  Color clr = get_lut_color(COLOR_BTN2);
  DrawTexturePro(sprites, rect_medal, dst, (Vector2){0}, 0, clr);
  rlPopMatrix();
}

void win_level_draw() {
  draw_win(C.modal, "LEVEL SELECTION");
  // draw_widget_frame(C.tb.box);
  // draw_widget_frame(C.tb_grp.box);
  textbox_draw(&C.tb);
  textbox_draw(&C.tb_grp);
  // btn_draw_text(&C.btn_campaign2, ui_get_scale(),
  //               TextFormat("%s Campaign", C.selected_group->name));

  label_draw(&C.campaign_title);

  btn_draw_text(&C.btn_choose, ui_get_scale(), "SELECT LEVEL");
  btn_draw_text(&C.btn_close, ui_get_scale(), "CLOSE");

  Texture2D sprites = ui_get_sprites();
  int bscale = ui_get_scale();
  // btn_draw_icon(&C.btn_msg[1], bscale, sprites, rect_book);

  LevelDef* lvl = C.selected_level;
  int j = 0;
  int nextra = arrlen(lvl->extra_content);
  for (int i = 0; i < nextra; i++) {
    LevelDefExtraItem* item = &lvl->extra_content[i];
    if (item->text) {
      btn_draw_icon(&C.btn_msg[j++], bscale, sprites, rect_bulbon);
    }
    if (item->wiki) {
      btn_draw_icon(&C.btn_msg[j++], bscale, sprites, rect_book);
    }
    if (item->tex.width > 0) {
      btn_draw_icon(&C.btn_msg[j++], bscale, sprites, rect_img);
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

  for (int i = 0; i < 10; i++) {
    Color c = BLACK;
    c.a = 150;
    DrawRectangleRec(C.btn_campaign[i].hitbox, c);
  }

  int nc = arrlen(r->group_order);
  for (int i = 0; i < 10; i++) {
    if (i < nc) {
      LevelGroup* g = r->group_order[i];
      const char* title = g->name;
      if (!g->can_choose) title = "???";
      Rectangle r = C.btn_campaign[i].hitbox;
      Rectangle rect = {0, 0, g->icon.width, g->icon.height};
      btn_draw_icon(&C.btn_campaign[i], bscale, g->icon, rect);
      Color c = BLACK;
      c.a = 150;
      DrawRectangleRec(C.med_camp[i], c);
      if (g->complete) {
        draw_medal(C.med_camp[i]);
      }
    }
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (i < nl) {
      LevelDef* ldef = levels[i];
      btn_draw_text(&C.btn_option[i], ui_get_scale(),
                    TextFormat("%s. %s", get_roman_number(i + 1), ldef->name));
      if (ldef->complete) {
        draw_medal(C.medals[i]);
      }
    }
  }

  if (ui_get_window() == WINDOW_LEVEL) {
    for (int i = 0; i < nextra; i++) {
      LevelDefExtraItem* item = &lvl->extra_content[i];
      if (item->text || item->tex.width > 0) {
        btn_draw_legend(&C.btn_msg[i], bscale, lvl->extra_content[i].title);
      }
      if (item->wiki) {
        TutorialItem* wiki_item = find_wiki_from_id(C.registry, item->wiki);
        btn_draw_legend(&C.btn_msg[i], bscale,
                        TextFormat("Wiki: %s", wiki_item->name));
      }
    }

    int nc = arrlen(r->group_order);
    for (int i = 0; i < nc; i++) {
      btn_draw_legend(&C.btn_campaign[i], bscale,
                      TextFormat("Campaign: %s", r->group_order[i]->name));
    }
  }
  label_draw(&C.level_title);
}
