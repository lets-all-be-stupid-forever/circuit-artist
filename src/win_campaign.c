#include "win_campaign.h"

#include "game_registry.h"
#include "i18n.h"
#include "layout.h"
#include "paths.h"
#include "stb_ds.h"
#include "ui.h"
#include "uifont.h"
#include "win_home.h"
#include "win_level.h"
#include "win_msg.h"

static struct {
  Layout* layout;
  Rectangle modal;
  Rectangle bg;
  Texture2D campbg;
  LevelGroup** camps;
  Btn btn_camp[50];
  Btn btn_back;
  int nc;
} C = {0};

static void update_layout() {
  layout_update_offset(C.layout);
  Layout* l = C.layout;
  C.modal = layout_rect(l, "window");
  C.bg = layout_rect(l, "bg");
  for (int i = 0; i < C.nc; i++) {
    C.btn_camp[i].hitbox = layout_rect(l, C.camps[i]->id);
  }
  C.btn_back.hitbox = layout_rectb(l, "btn_back");
}

void win_campaign_init() {
  C.camps = getreg()->group_order;
  C.nc = arrlen(getreg()->group_order);
  C.layout = easy_load_layout("campaign");
  char* path = get_asset_path("imgs/campbg.png");
  C.campbg = LoadTexture(path);
  free(path);
}

void win_campaign_open() {
  ui_winpush(WINDOW_CAMPAIGN);
  update_layout();
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

void win_campaign_update() {
  update_layout();
  bool escape = IsKeyPressed(KEY_ESCAPE);
  if (escape || btn_update(&C.btn_back)) {
    ui_winpop();
    win_home_open();
    return;
  }
  for (int i = 0; i < C.nc; i++) {
    C.btn_camp[i].disabled = !C.camps[i]->can_choose;
    if (btn_update(&C.btn_camp[i])) {
      LevelDef* ldef = find_best_level(C.camps[i]);
      ui_winpop();
      win_level_open(ldef->group);
      return;
    }
    if (btn_right_click(&C.btn_camp[i])) {
      win_msg_open_text(C.camps[i]->desc, NULL);
    }
    if (C.btn_camp[i].hover && !C.btn_camp[i].disabled) {
      ui_set_cursor(MOUSE_POINTER);
    }
  }
}

void win_campaign_draw() {
  ClearBackground(BLANK);
  draw_default_tiled_screen();
  draw_win(C.modal, T.select_campaign);
  Texture2D sprites = ui_get_sprites();
  DrawRectangleRec(C.bg, (Color){0, 0, 0, 150});

  {
    Rectangle source = {0, 0, C.campbg.width, C.campbg.height};
    Rectangle target = {0, 0, C.campbg.width, C.campbg.height};
    rlPushMatrix();
    rlTranslatef(C.bg.x, C.bg.y, 0);
    rlScalef(2, 2, 1);
    DrawTexturePro(C.campbg, source, target, (Vector2){0}, 0, WHITE);
    rlPopMatrix();
  }

  draw_frame(C.bg);
  int bscale = 2;
  for (int i = 0; i < C.nc; i++) {
    LevelGroup* g = C.camps[i];
    Rectangle r = C.btn_camp[i].hitbox;
    Color c = BLACK;
    bool done = g->complete;
    c.a = 150;
    Rectangle med = (Rectangle){r.x + 4, r.y + 4, 34, 34};
    const char* title = g->name;
    if (!g->can_choose) title = "???";
    Rectangle rect = {0, 0, g->icon.width, g->icon.height};

    bool dis = C.btn_camp[i].disabled;
    if (true) {
      rlPushMatrix();
      rlTranslatef(r.x, r.y, 0);
      rlScalef(2, 2, 1);
      int p = 5;
      rlTranslatef(-p, -p, 0);
      if (dis) {
        Rectangle source = {846 - p, 262 - p, 130 + 2 * p, 72 + 2 * p};
        Rectangle dest = {0, 0, source.width, source.height};
        DrawTexturePro(sprites, source, dest, (Vector2){0}, 0, WHITE);
      } else if (done) {
        Rectangle source = {752 - p, 179 - p, 130 + 2 * p, 72 + 2 * p};
        Rectangle dest = {0, 0, source.width, source.height};
        DrawTexturePro(sprites, source, dest, (Vector2){0}, 0, WHITE);
      } else {
        Rectangle source = {608 - p, 224 - p, 130 + 2 * p, 72 + 2 * p};
        Rectangle dest = {0, 0, source.width, source.height};
        DrawTexturePro(sprites, source, dest, (Vector2){0}, 0, WHITE);
      }

      rlPopMatrix();
    }
    Texture2D texture = g->icon;
    Rectangle source = rect;
    int lh = 48;
    Rectangle top = {
        r.x,
        r.y,
        r.width,
        r.height - lh,
    };
    Rectangle bot = {
        r.x,
        r.y + r.height - lh,
        r.width,
        lh,
    };

    Btn* b = &C.btn_camp[i];
    int s = 2;
    int fx = (top.x + top.width / 2) - s * source.width / 2;
    int fy = (top.y + top.height / 2) - s * source.height / 2;
    int x = top.x;
    int y = top.y;
    int w = top.width;
    int h = top.height;
    Color k = CA_WHITE;
    if (dis) k = CA_GRAYDARK;

    rlPushMatrix();
    rlTranslatef(fx, fy, 0);
    rlScalef(s, s, 1);
    if (!dis) {
      DrawTextureRec(texture, source, (Vector2){1, 1}, BLACK);
    }
    DrawTextureRec(texture, source, (Vector2){0, 0}, k);
    rlPopMatrix();
    custom_label_draw_centered(bot, g->name, k, dis ? BLANK : BLACK);
    if (g->complete) {
      if (!dis) {
        draw_medal((Rectangle){med.x + 2, med.y + 2, med.width, med.height},
                   BLACK);
      }
      draw_medal(med, k);
    }
  }

  btn_draw_text(&C.btn_back, T.back);
}

