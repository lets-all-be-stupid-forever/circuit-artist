#include "win_level.h"

#include "colors.h"
#include "font.h"
#include "game_registry.h"
#include "layout.h"
#include "stb_ds.h"
#include "tutorial.h"
#include "ui.h"
#include "utils.h"
#include "wabout.h"
#include "widgets.h"
#include "win_campaign.h"
#include "win_msg.h"

#define NUM_BUTTONS 42

static struct {
  Level* lvl; /* Active level */
  GameRegistry* registry;
  LevelGroup* selected_group;
  LevelDef* selected_level;
  LevelDef* active_level;
  char* mod_root;
  Rectangle modal;
  Rectangle group_panel;
  Rectangle camp_panel;
  Rectangle buttons;
  Btn btn_option[NUM_BUTTONS];
  Label campaign_title;
  // Btn btn_campaign2;
  Btn btn_choose;
  Btn btn_close;
  Btn btn_campaign[10];
  Rectangle med_camp[10];
  Btn btn_msg[8];
  Rectangle medals[NUM_BUTTONS];
  Label level_title;
  Textbox tb;
  Textbox tb_grp;
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
  C.modal = WIN_LEVEL_win_level;
  Vector2 off = find_modal_off(C.modal);
  C.modal = roff(off, WIN_LEVEL_win_level);

  C.buttons = roff(off, WIN_LEVEL_LEVELS_PANEL);
  C.level_title.hitbox = roff(off, WIN_LEVEL_LEVEL_TITLE);

  int x0 = C.buttons.x + 2;
  int y0 = C.buttons.y + 2;
  int cols = 1;
  int bw = C.buttons.width;
  int bh = 17 * 2;
  C.camp_panel = roff(off, WIN_LEVEL_CAMP_PANEL);
  C.btn_msg[0].hitbox = roff(off, WIN_LEVEL_msg1);
  C.btn_msg[1].hitbox = roff(off, WIN_LEVEL_msg2);
  C.btn_msg[2].hitbox = roff(off, WIN_LEVEL_msg3);
  C.btn_msg[3].hitbox = roff(off, WIN_LEVEL_msg4);
  C.btn_msg[4].hitbox = roff(off, WIN_LEVEL_msg5);
  C.btn_msg[5].hitbox = roff(off, WIN_LEVEL_msg6);
  C.btn_msg[6].hitbox = roff(off, WIN_LEVEL_msg7);
  C.btn_msg[7].hitbox = roff(off, WIN_LEVEL_msg8);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    int xx = i % cols;
    int yy = i / cols;
    Rectangle r = (Rectangle){
        .x = xx * (bw + 2) + x0,
        .y = yy * (bh + 2) + y0,
        .width = bw,
        .height = bh,
    };
    C.btn_option[i].hitbox = (Rectangle){
        r.x,
        r.y,
        r.width - 20 * 2 - 4,
        r.height,
    };
    C.medals[i] = (Rectangle){
        r.x + r.width - 20 * 2,
        r.y,
        20 * 2,
        r.height,
    };
  }
  C.btn_choose.hitbox = roff(off, WIN_LEVEL_choose);
  C.btn_close.hitbox = roff(off, WIN_LEVEL_close);
  // C.btn_campaign2.hitbox = roff(off, WIN_LEVEL_campaign);
  C.campaign_title.hitbox = roff(off, WIN_LEVEL_campaign);

  int c_cols = 5;
  RectGridParam g = {
      .x = C.camp_panel.x,
      .y = C.camp_panel.y + 30,
      .w = 90,
      .h = 80,
      .rows = 2,
      .cols = c_cols,
      .spacex = 4,
      .spacey = 8,
  };

  for (int i = 0; i < 10; i++) {
    Rectangle r = grid_rect(g, i / c_cols, i % c_cols);
    int mw = 40;
    C.btn_campaign[i].hitbox = r;
    C.med_camp[i] = (Rectangle){
        r.x + 4,
        r.y + 4,
        mw,
        mw,
    };
  }

  textbox_set_box(&C.tb, roff(off, WIN_LEVEL_level_text));
  textbox_set_box(&C.tb_grp, roff(off, WIN_LEVEL_camp_text));
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

  C.active_level = NULL;   /* -1 means not loaded */
  C.selected_level = NULL; /* -1 means not loaded */
}

static void select_first_level() {
  win_level_set_sel(C.selected_group->levels[0]);
}

static void select_first_available_level() {
  LevelGroup* lg = C.selected_group;
  int nl = arrlen(lg->levels);
  for (int i = 0; i < nl; i++) {
    int j = nl - i - 1;
    if (lg->levels[j]->can_choose) {
      win_level_set_sel(lg->levels[j]);
      return;
    }
  }
}

void win_level_open() {
  ui_winpush(WINDOW_LEVEL);
  update_layout();
  /* When window opens, it puts the active level as the selected one */
  LevelDef* sel = C.active_level;
  if (!sel) {
    sel = C.registry->group_order[0]->levels[0];
  }
  win_level_set_sel(sel);
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
        // about_open(item->title, item->text, NULL);
        win_msg_open_text(item->text, NULL);
      }
      if (item->wiki) {
        tutorial_open_on_item(item->wiki);
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
      C.btn_option[i].disabled = !ldef->can_choose;
      C.btn_option[i].toggled = (C.selected_level == ldef);
    }
  }

  if (btn_update(&C.btn_choose)) {
    level_load(C.selected_level);
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
  }
  label_draw(&C.level_title);
}

Level* getlevel() { return C.lvl; }

void level_load(LevelDef* ldef) {
  /* Doesnt need to do anything if active level is same */
  if (ldef == C.active_level) return;
  C.active_level = ldef;
  int n = arrlen(ldef->kernels);
  if (C.lvl) level_destroy(C.lvl);
  C.lvl = calloc(1, sizeof(Level));
  bool ok = level_init(C.lvl, ldef);
  C.lvl->ldef = ldef;
}

void level_load_default() {
  LevelDef* ldef = C.registry->group_order[0]->levels[0];
  level_load(ldef);
}
