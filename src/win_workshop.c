#include "win_workshop.h"

#include "font.h"
#include "game_registry.h"
#include "i18n.h"
#include "layout.h"
#include "rlgl.h"
#include "stb_ds.h"
#include "stdio.h"
#include "steam.h"
#include "ui.h"
#include "uifont.h"
#include "utils.h"
#include "widgets.h"
#include "win_workshopdet.h"

#define NUM_SLOTS 24
#define STEAM_PAGE_SIZE 50

typedef enum {
  DISPLAY_WORKSHOP = 0,
  DISPLAY_SUBSCRIBED,
  DISPLAY_MY_UPLOADS,
} WorkshopDisplayMode;

static struct {
  void* q;
  Rectangle modal;
  Layout* layout;
  LineEdit edit;
  Btn btn_prv;
  Btn btn_nxt;
  Btn btn_sort[3];  // trending, recent, votes
  Btn btn_steam;
  Btn btn_text;
  Btn btn_subscribed;
  Btn btn_myuploads;
  Btn btn_close;
  char search_text[256];  // active text filter; empty = sort mode
  WorkshopDisplayMode display_mode;
  Label lbl_showing;
  Label lbl_no_results;
  Btn opts[NUM_SLOTS];
  void* http[NUM_SLOTS];
  u32 istate[NUM_SLOTS];
  Texture2D thumbnails[NUM_SLOTS];

  QueryResult* cached_pages;  // stb_ds array; items live until close
  int num_cached;             // total items accumulated across all pages
  int total;                  // total items reported by Steam
  int ui_page;
  WorkshopSortMode sort_mode;
  char* level_id;  // NULL = generic search, non-NULL = solutions for level
} C = {0};

static QueryResultItem* get_cached_item(int i) {
  if (i < 0 || i >= C.num_cached) return NULL;
  int page = i / STEAM_PAGE_SIZE;
  int idx = i % STEAM_PAGE_SIZE;
  if (page >= arrlen(C.cached_pages)) return NULL;
  if (idx >= C.cached_pages[page].num_items) return NULL;
  return &C.cached_pages[page].items[idx];
}

static void update_steam_item_states() {
  int start = C.ui_page * NUM_SLOTS;
  for (int i = 0; i < NUM_SLOTS; i++) {
    QueryResultItem* item = get_cached_item(start + i);
    C.istate[i] = item ? steam_item_state(item->file_id) : ITEM_STATE_NONE;
  }
}

static void update_layout() {
  layout_update_offset(C.layout);
  C.modal = layout_rect(C.layout, "window");
  C.btn_prv.hitbox = layout_rect(C.layout, "btn_prev");
  C.btn_nxt.hitbox = layout_rect(C.layout, "btn_next");
  C.edit.hitbox = layout_rect(C.layout, "text");
  C.btn_steam.hitbox = layout_rect(C.layout, "btn_steam");
  C.btn_text.hitbox = layout_rect(C.layout, "btn_text");
  C.btn_subscribed.hitbox = layout_rect(C.layout, "btn_subscribed");
  C.btn_myuploads.hitbox = layout_rect(C.layout, "btn_myuploads");
  C.btn_close.hitbox = layout_rect(C.layout, "btn_close");
  C.btn_sort[WORKSHOP_SORT_TRENDING].hitbox =
      layout_rect(C.layout, "btn_trending");
  C.btn_sort[WORKSHOP_SORT_RECENT].hitbox = layout_rect(C.layout, "btn_recent");
  C.btn_sort[WORKSHOP_SORT_VOTES].hitbox = layout_rect(C.layout, "btn_votes");
  C.lbl_showing.hitbox = layout_rect(C.layout, "showing");
  Rectangle r = layout_rect(C.layout, "rects");
  C.lbl_no_results.hitbox = r;
  int cols = 6;
  for (int i = 0; i < NUM_SLOTS; i++) {
    int cx = i % cols;
    int cy = i / cols;
    C.opts[i].hitbox = (Rectangle){r.x + cx * 128, r.y + cy * 128, 128, 128};
  }
}

void win_workshop_init() {
  C.layout = easy_load_layout("workshop");
  update_layout();
}

static void free_thumbnails() {
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (C.http[i]) {
      steam_http_destroy(C.http[i]);
      C.http[i] = NULL;
    }
    if (C.thumbnails[i].id) {
      UnloadTexture(C.thumbnails[i]);
      C.thumbnails[i] = (Texture2D){0};
    }
  }
}

static void fetch_thumbnails_for_current_page() {
  int start = C.ui_page * NUM_SLOTS;
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (C.http[i] || C.thumbnails[i].id) continue;
    QueryResultItem* item = get_cached_item(start + i);
    if (item && item->url && item->url[0]) {
      C.http[i] = steam_http_call(item->url);
    }
  }
}

static void ensure_items_for_page(int page) {
  if (C.q) return;
  int needed = (page + 1) * NUM_SLOTS;
  if (C.num_cached >= needed) return;
  if (C.total > 0 && C.num_cached >= C.total) return;
  int next_steam_page = arrlen(C.cached_pages) + 1;
  if (C.level_id) {
    C.q = steam_query_solution(C.level_id, next_steam_page, C.sort_mode);
  } else if (C.display_mode == DISPLAY_SUBSCRIBED) {
    C.q = steam_query_subscribed(next_steam_page);
  } else if (C.display_mode == DISPLAY_MY_UPLOADS) {
    C.q = steam_query_my_uploads(next_steam_page);
  } else {
    const char* txt = (C.sort_mode == WORKSHOP_SORT_TEXT) ? C.search_text : "";
    C.q = steam_query_call_all(txt, next_steam_page, C.sort_mode);
  }
}

static void free_cache() {
  free_thumbnails();
  for (int i = 0; i < arrlen(C.cached_pages); i++) {
    unload_query_results(&C.cached_pages[i]);
  }
  arrfree(C.cached_pages);
  C.cached_pages = NULL;
  C.num_cached = 0;
  C.total = 0;
  if (C.q) {
    steam_query_destroy(C.q);
    C.q = NULL;
  }
}

static void reset_search() {
  C.ui_page = 0;
  free_cache();
  ensure_items_for_page(0);
}

void win_workshop_open(const char* level_id) {
  ui_winpush(WINDOW_WORKSHOP);
  update_layout();
  C.sort_mode = WORKSHOP_SORT_TRENDING;
  C.display_mode = DISPLAY_WORKSHOP;
  C.search_text[0] = '\0';
  lineedit_init(&C.edit);
  lineedit_set_focus(&C.edit, true);
  free(C.level_id);
  C.level_id = level_id ? clone_string(level_id) : NULL;
  reset_search();
}

void win_workshop_on_close() {
  free_cache();
  free(C.level_id);
  C.level_id = NULL;
}

static void update_query() {
  if (!C.q) return;
  QueryResult r = {0};
  if (!steam_query_update(C.q, &r)) return;
  arrput(C.cached_pages, r);
  C.num_cached += r.num_items;
  C.total = r.total;
  steam_query_destroy(C.q);
  C.q = NULL;
  fetch_thumbnails_for_current_page();
  ensure_items_for_page(C.ui_page);
}

static int get_num_ui_pages() { return (C.total + NUM_SLOTS - 1) / NUM_SLOTS; }

static void update_thumbnails() {
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (!C.http[i]) continue;
    Image img = {0};
    if (!steam_http_update(C.http[i], &img)) continue;
    if (img.width > 0) {
      C.thumbnails[i] = LoadTextureFromImage(img);
      UnloadImage(img);
    }
    steam_http_destroy(C.http[i]);
    C.http[i] = NULL;
  }
}

static void navigate_to_page(int page) {
  C.ui_page = page;
  free_thumbnails();
  fetch_thumbnails_for_current_page();
  ensure_items_for_page(page);
}

void win_workshop_update() {
  update_layout();
  update_query();
  update_steam_item_states();
  update_thumbnails();

  if (IsKeyPressed(KEY_ESCAPE) || btn_update(&C.btn_close)) {
    ui_winpop();
    return;
  }

  int num_pages = get_num_ui_pages();
  C.btn_prv.disabled = C.ui_page == 0;
  C.btn_nxt.disabled = num_pages == 0 || C.ui_page >= num_pages - 1;

  if (btn_update(&C.btn_nxt)) navigate_to_page(C.ui_page + 1);

  if (btn_update(&C.btn_prv)) navigate_to_page(C.ui_page - 1);

  bool workshop_mode = C.display_mode == DISPLAY_WORKSHOP;

  // Subscribed / my-uploads mode buttons
  if (btn_update(&C.btn_subscribed) && C.display_mode != DISPLAY_SUBSCRIBED) {
    C.display_mode = DISPLAY_SUBSCRIBED;
    C.search_text[0] = '\0';
    lineedit_set_text(&C.edit, "");
    reset_search();
  }
  if (btn_update(&C.btn_myuploads) && C.display_mode != DISPLAY_MY_UPLOADS) {
    C.display_mode = DISPLAY_MY_UPLOADS;
    C.search_text[0] = '\0';
    lineedit_set_text(&C.edit, "");
    reset_search();
  }

  // Sort buttons: mutually exclusive with text search
  for (int i = 0; i < 3; i++) {
    if (btn_update(&C.btn_sort[i]) &&
        (C.sort_mode != (WorkshopSortMode)i || !workshop_mode)) {
      C.sort_mode = (WorkshopSortMode)i;
      C.display_mode = DISPLAY_WORKSHOP;
      C.search_text[0] = '\0';
      lineedit_set_text(&C.edit, "");
      reset_search();
    }
  }

  // Text search: sets TEXT mode, deactivates sort buttons
  lineedit_update(&C.edit);
  lineedit_set_focus(&C.edit, true);
  bool search_triggered = btn_update(&C.btn_text) || IsKeyPressed(KEY_ENTER);
  if (search_triggered) {
    strncpy(C.search_text, lineedit_get_text(&C.edit),
            sizeof(C.search_text) - 1);
    C.search_text[sizeof(C.search_text) - 1] = '\0';
    C.sort_mode = WORKSHOP_SORT_TEXT;
    C.display_mode = DISPLAY_WORKSHOP;
    reset_search();
  }

  C.btn_steam.disabled = C.level_id != NULL;
  if (btn_update(&C.btn_steam)) steam_open_overlay_blueprints();

  int start = C.ui_page * NUM_SLOTS;
  for (int i = 0; i < NUM_SLOTS; i++) {
    QueryResultItem* item = get_cached_item(start + i);
    C.opts[i].hidden = item == NULL;
    bool can_subscribe = false;
    bool can_unsubscribe = false;
    if (!C.opts[i].hidden) {
      u32 state = C.istate[i];
      can_subscribe = (state & ITEM_STATE_SUBSCRIBED) == 0;
      can_unsubscribe = (state & ITEM_STATE_SUBSCRIBED) != 0;
      if (C.opts[i].hover) ui_set_cursor(MOUSE_POINTER);
    }
    if (btn_update(&C.opts[i]) && item) {
      if (can_subscribe)
        steam_subscribe_item(item->file_id);
      else if (can_unsubscribe)
        steam_unsubscribe_item(item->file_id);
    }
  }
}

void win_workshop_draw() {
  const char* title;
  if (C.level_id) {
    title = TextFormat(T.workshop_title_solutions,
                       get_level_name_by_id(C.level_id));
  } else {
    title = T.workshop_title;
  }
  draw_win(C.modal, title);
  btn_draw_icon(&C.btn_nxt, rect_right);
  btn_draw_icon(&C.btn_prv, rect_left);

  const char* sort_labels[] = {T.workshop_sort_trending, T.workshop_sort_recent,
                               T.workshop_sort_votes};
  bool workshop_mode_draw = C.display_mode == DISPLAY_WORKSHOP;
  for (int i = 0; i < 3; i++) {
    C.btn_sort[i].toggled =
        workshop_mode_draw &&
        C.sort_mode ==
            (WorkshopSortMode)i;  // not toggled for WORKSHOP_SORT_TEXT
    btn_draw_text(&C.btn_sort[i], sort_labels[i]);
  }
  C.btn_subscribed.toggled = C.display_mode == DISPLAY_SUBSCRIBED;
  C.btn_myuploads.toggled = C.display_mode == DISPLAY_MY_UPLOADS;
  btn_draw_text(&C.btn_close, T.close);
  btn_draw_text(&C.btn_steam, T.workshop_browse_steam);
  btn_draw_text(&C.btn_subscribed, T.workshop_subscribed);
  btn_draw_text(&C.btn_myuploads, T.workshop_my_uploads);
  lineedit_draw(&C.edit);
  btn_draw_text(&C.btn_text, T.workshop_search);

  int num_pages = get_num_ui_pages();
  bool no_results = arrlen(C.cached_pages) > 0 && num_pages == 0;
  if (arrlen(C.cached_pages) == 0) {
    label_set_text(&C.lbl_showing, "- / -");
  } else if (no_results) {
    label_set_text(&C.lbl_showing, "0 / 0");
  } else {
    label_set_text(&C.lbl_showing,
                   TextFormat("%d / %d", C.ui_page + 1, num_pages));
  }
  label_draw_centered(&C.lbl_showing);

  if (arrlen(C.cached_pages) == 0) {
    label_set_text(&C.lbl_no_results, T.workshop_fetching);
    label_draw_centered(&C.lbl_no_results);
  } else if (no_results) {
    label_set_text(&C.lbl_no_results, T.workshop_no_results);
    label_draw_centered(&C.lbl_no_results);
  }

  int start = C.ui_page * NUM_SLOTS;
  Texture sprites = ui_get_sprites();
  for (int i = 0; i < NUM_SLOTS; i++) {
    Rectangle r = C.opts[i].hitbox;
    QueryResultItem* item = get_cached_item(start + i);
    Color bg = BLACK;
    bg.a = 150;
    DrawRectangleRec(r, bg);
    if (C.thumbnails[i].id) {
      DrawTexturePro(
          C.thumbnails[i],
          (Rectangle){0, 0, C.thumbnails[i].width, C.thumbnails[i].height}, r,
          (Vector2){0}, 0, WHITE);
    }
    if (item) {
      u32 state = C.istate[i];
      Color bc = BLANK;
      if ((state & ITEM_STATE_SUBSCRIBED) != 0) {
        bc = (state & ITEM_STATE_INSTALLED) != 0 ? GREEN : YELLOW;
      }
      DrawTexturePro(sprites, (Rectangle){256, 208, 32, 32}, r, (Vector2){0}, 0,
                     bc);
      if (C.opts[i].hover) {
        DrawTexturePro(sprites, (Rectangle){304, 208, 32, 32}, r, (Vector2){0},
                       0, CA_WHITE);
        if (state & ITEM_STATE_SUBSCRIBED) {
          const char* txt = T.workshop_subscribed_overlay;
          v2 sz = uifont_text_size(txt);
          float tx = r.x + (r.width - sz.x) / 2;
          float ty = r.y + (r.height - sz.y) / 2;
          uifont_draw_texture_outlined(txt, tx, ty, CA_ORANGE, BLACK);
        }
      }
    }
  }

  // Hover legend — drawn last so it appears on top
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (!C.opts[i].hover) continue;
    QueryResultItem* item = get_cached_item(start + i);
    if (!item) continue;
    int k = 100;
    Color dim = {k, k, k, 255};
    LegendBuilder lb = {0};
    lb_init(&lb);
    lb_add_line(&lb, item->title ? item->title : "", 4, CA_WHITE);
    const char* author = steam_get_author_name(item->owner_id);
    if (author && author[0]) {
      lb_add_line(&lb, TextFormat(T.by_author, author), 2, CA_GRAYDARK);
    }
    lb_render(&lb, C.opts[i].hitbox);
    unload_lb(&lb);
    break;
  }
}
