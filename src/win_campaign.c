#include "win_campaign.h"

#include "common.h"
#include "font.h"
#include "game_registry.h"
#include "layout.h"
#include "stb_ds.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"
#include "win_level.h"

static struct {
  GameRegistry* registry;
  Rectangle modal;
  Btn options[20];
} C = {0};

static void update_layout() {
  int cols = 3;
  int rows = 2;
  int bw = 260;
  int bh = 140;
  int padx = 30;
  int pady = 20;
  int offy = 50;
  int marx = 2 * padx;
  int mary = 2 * pady;

  int tw = cols * (bw + padx) - padx + 2 * marx;
  int th = rows * (bh + pady) - pady + 2 * mary + offy;

  C.modal = (Rectangle){0, 0, tw, th};
  Vector2 off = find_modal_off(C.modal);
  C.modal = roff(off, C.modal);

  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      int bx = marx + (bw + padx) * x;
      int by = offy + mary + (bh + pady) * y;
      Rectangle b = (Rectangle){
          .x = bx,
          .y = by,
          .width = bw,
          .height = bh,
      };
      int idx = y * cols + x;
      C.options[idx].hitbox = roff(off, b);
    }
  }
}

void win_campaign_init(GameRegistry* r) { C.registry = r; }

void win_campaign_open() {
  ui_winpush(WINDOW_CAMPAIGN);
  update_layout();
}

void win_campaign_update() {
  update_layout();

  GameRegistry* r = C.registry;
  int nc = arrlen(r->group_order);

  if (IsKeyPressed(KEY_ESCAPE)) {
    ui_winpop();
    return;
  }

  Vector2 mouse = GetMousePosition();
  for (int i = 0; i < nc; i++) {
    Rectangle h = C.options[i].hitbox;
    bool disabled = !r->group_order[i]->can_choose;
    if (rect_hover(h, mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !disabled && ui_get_hit_count() == 0) {
      ui_inc_hit_count();
      ui_winpop();
      win_level_set_campaign(i);
      return;
    }
  }

  for (int i = 0; i < nc; i++) {
    C.options[i].disabled = !r->group_order[i]->can_choose;
  }
}

void win_campaign_draw() {
  draw_win(C.modal, "CAMPAIGN SELECTION");
  GameRegistry* r = C.registry;
  int nc = arrlen(r->group_order);

  for (int i = 0; i < 6; i++) {
    if (i < nc) {
      Color c = BLACK;
      Rectangle h = C.options[i].hitbox;
      DrawRectangleRec(h, c);
      LevelGroup* g = r->group_order[i];
      const char* title = g->name;
      if (!g->can_choose) title = "???";
      font_draw_texture(title, h.x, h.y, WHITE);
    } else {
      Color c = BLACK;
      c.a = 100;
      DrawRectangleRec(C.options[i].hitbox, c);
    }
  }
}
