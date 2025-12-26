#include "tutorial.h"

#include "assert.h"
#include "common.h"
#include "config.h"
#include "font.h"
#include "game_registry.h"
#include "stb_ds.h"
#include "stdlib.h"
#include "string.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

static struct {
  GameRegistry* registry;
  bool inited;
  Textbox tb;
  Listbox lb;
  Listbox lb0;
  int topic_sel;
  int item_sel;
  Rectangle* layout;
  Rectangle modal;
  bool opened_this_frame;
  bool closed;
  Btn btn_close;
  Btn btn_topic[10];
} C = {0};

static void update_layout() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int ww = C.layout[0].width;
  int wh = C.layout[0].height;
  int s = ui_get_scale();
  int x0 = (sw - ww) / 2;
  int y0 = (sh - wh) / 2;
  x0 = s * (x0 / s);
  y0 = s * (y0 / s);
  Vector2 off = {x0, y0};
  Rectangle* l = C.layout;
  C.modal = roff(off, *l++);
  for (int i = 0; i < 7; i++) {
    C.btn_topic[i].hitbox = roff(off, *l++);
  }
  listbox_set_box(&C.lb, roff(off, *l++));
  textbox_set_box(&C.tb, roff(off, *l++));
  C.btn_close.hitbox = roff(off, *l++);
}

static TutorialTopic* get_topics() { return C.registry->topics; }

static void set_sel(int topic, int item) {
  TutorialTopic* topics = get_topics();
  if (topic != C.topic_sel) {
    C.topic_sel = topic;
    C.item_sel = -1;
    listbox_clear(&C.lb);
    TutorialItem* items = topics[C.topic_sel].items;
    for (int i = 0; i < arrlen(items); i++) {
      listbox_add_row_icon(&C.lb, items[i].name, items[i].icon);
    }
  }
  if (item != C.item_sel) {
    C.item_sel = item;
    TutorialItem* item = &topics[C.topic_sel].items[C.item_sel];
    textbox_set_content(&C.tb, item->desc, item->desc_imgs);
  }
}

void tutorial_init(GameRegistry* r) {
  C.registry = r;
  C.layout = parse_layout("../assets/layout/circ_layout1.png");
  C.topic_sel = -1;
  C.item_sel = -1;
  textbox_init(&C.tb);
  listbox_init(&C.lb);
  C.lb.row_pad = 5;

  update_layout();
  set_sel(0, 0);
}

void tutorial_open() {
  ui_winpush(WINDOW_TUTORIAL);
  C.opened_this_frame = true;
  C.closed = false;
}

void tutorial_update() {
  update_layout();
  int ntopic = arrlen(get_topics());
  for (int i = 0; i < 10; i++) {
    C.btn_topic[i].hidden = i >= ntopic;
  }
  if (IsKeyPressed(KEY_TAB) && !C.opened_this_frame) {
    ui_winpop();
    return;
  }
  C.opened_this_frame = false;

  // Cancel button: we close without performing any action.
  if (btn_update(&C.btn_close) || IsKeyPressed(KEY_ESCAPE)) {
    ui_winpop();
    return;
  }

  // Topic buttons update
  for (int i = 0; i < ntopic; i++) {
    if (btn_update(&C.btn_topic[i])) {
      if (i != C.topic_sel) {
        set_sel(i, 0);
      }
    }
  }
  for (int i = 0; i < 10; i++) {
    C.btn_topic[i].toggled = i == C.topic_sel;
  }

  listbox_update(&C.lb);
  textbox_update(&C.tb);

  // Listbox rows
  int hit = C.lb.row_hit;
  if (hit >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      C.item_sel != hit) {
    set_sel(C.topic_sel, hit);
  }
}

static TutorialItem* get_item() {
  return &get_topics()[C.topic_sel].items[C.item_sel];
}

void tutorial_draw() {
  draw_win(C.modal, "WIKI");
  // DrawRectangleRec(C.tb.box, BLACK);
  // draw_widget_frame(C.tb.box);

  textbox_draw(&C.tb);
  listbox_draw(&C.lb, C.item_sel);
  TutorialTopic* topics = get_topics();
  int ntopic = arrlen(topics);
  int s = ui_get_scale();
  for (int i = 0; i < ntopic; i++) {
    btn_draw_icon(&C.btn_topic[i], s, topics[i].icon.tex,
                  topics[i].icon.region);
  }
  for (int i = 0; i < ntopic; i++) {
    btn_draw_legend(&C.btn_topic[i], s, topics[i].name);
  }
  btn_draw_text(&C.btn_close, ui_get_scale(), "CLOSE");
}

void tutorial_open_on_item(const char* item_id) {
  tutorial_open();
  TutorialTopic* topics = get_topics();
  int nt = arrlen(topics);
  for (int it = 0; it < nt; it++) {
    int ni = arrlen(topics[it].items);
    for (int ii = 0; ii < ni; ii++) {
      if (strcmp(topics[it].items[ii].id, item_id) == 0) {
        set_sel(it, ii);
        return;
      }
    }
  }
}
