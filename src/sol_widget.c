#include "sol_widget.h"

#include "stb_ds.h"
#include "stdlib.h"
#include "steam.h"
#include "string.h"
#include "ui.h"
#include "utils.h"
#include "win_blueprint.h"
#include "win_bpdetail.h"
#include "win_workshop.h"

void sol_widget_update_layout(SolWidget* s, Layout* l) {
  s->btn_prv.hitbox = layout_rect(l, "btn_sol_prv");
  s->btn_nxt.hitbox = layout_rect(l, "btn_sol_nxt");
  s->btn_browse.hitbox = layout_rect(l, "btn_browse_sol");
  s->sol[0].hitbox = layout_rect(l, "sol1");
  s->sol[1].hitbox = layout_rect(l, "sol2");
  s->sol[2].hitbox = layout_rect(l, "sol3");
  s->sol[3].hitbox = layout_rect(l, "sol4");
}

static Blueprint* get_bp_for_slot(SolWidget* s, int i) {
  int n = arrlen(s->bps);
  int j = s->page * 4 + i;
  if (j >= n) return NULL;
  return s->bps[j];
}

void sol_widget_draw(SolWidget* s) {
  Color bg = {0, 0, 0, 150};
  DrawRectangleRec(s->sol[0].hitbox, bg);
  DrawRectangleRec(s->sol[1].hitbox, bg);
  DrawRectangleRec(s->sol[2].hitbox, bg);
  DrawRectangleRec(s->sol[3].hitbox, bg);
  DrawRectangleRec(s->btn_nxt.hitbox, bg);
  DrawRectangleRec(s->btn_prv.hitbox, bg);
  DrawRectangleRec(s->btn_browse.hitbox, bg);

  for (int i = 0; i < 4; i++) {
    Blueprint* bp = get_bp_for_slot(s, i);
    if (bp) {
      blueprint_draw(&s->sol[i], bp, 1);
    }
  }
  Texture sprites = ui_get_sprites();
  btn_draw_icon(&s->btn_prv, 2, sprites, rect_left);
  btn_draw_icon(&s->btn_nxt, 2, sprites, rect_right);
  btn_draw_icon(&s->btn_browse, 2, sprites, rect_steam);
}

static int get_num_pages(SolWidget* s) {
  int r = (arrlen(s->bps) + 3) / 4;
  if (r == 0) r = 1;
  return r;
}

void sol_widget_draw_leg(SolWidget* s) {
  for (int i = 0; i < 4; i++) {
    Blueprint* bp = get_bp_for_slot(s, i);
    if (bp) {
      blueprint_draw_leg(&s->sol[i], bp, 1);
    }
  }
}

static void update_blueprint_list(SolWidget* s) {
  arrsetlen(s->bps, 0);
  if (s->level_id) {
    get_linked_blueprints(&s->bps, s->level_id);
  }
  // 5 --> 2
  int npage = get_num_pages(s);
  if (s->page > npage - 1) s->page = npage - 1;
}

void sol_widget_set_level_id(SolWidget* s, const char* level_id) {
  free(s->level_id);
  s->level_id = clone_string(level_id);
  s->page = 0;
  update_blueprint_list(s);
  s->btn_browse.disabled = true;
  if (level_id) {
    bool is_local = starts_with(level_id, "local:");
    s->btn_browse.disabled = !is_steam_on() | is_local;
  }
}

void sol_widget_update(SolWidget* s) {
  update_blueprint_list(s);
  int npage = get_num_pages(s);
  s->btn_prv.disabled = npage <= 1;
  s->btn_nxt.disabled = npage <= 1;
  if (btn_update(&s->btn_prv)) {
    s->page = (s->page + npage - 1) % npage;
  }
  if (btn_update(&s->btn_nxt)) {
    s->page = (s->page + 1) % npage;
  }
  if (btn_update(&s->btn_browse)) {
    win_workshop_open(s->level_id);
  }
  for (int i = 0; i < 4; i++) {
    btn_update(&s->sol[i]);
    if (s->sol[i].hover) {
      Blueprint* bp = get_bp_for_slot(s, i);
      if (!bp) continue;
      ui_set_cursor(MOUSE_POINTER);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Blueprint* bp = get_bp_for_slot(s, i);
        win_bpdetail_open(bp, BPDETAIL_LOADWLEVEL);
      }
    };
  }
}

void sol_widget_init(SolWidget* s) { *s = (SolWidget){0}; }
