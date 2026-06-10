#include "win_customlvl.h"

#include "errno.h"
#include "fs.h"
#include "game_registry.h"
#include "i18n.h"
#include "layout.h"
#include "paths.h"
#include "sol_widget.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "wdialog.h"
#include "win_main.h"
#include "win_pubform.h"
#include "win_wiki.h"

typedef enum {
  PAGE_OFFICIAL = 0,
  PAGE_LOCAL = 1,
  PAGE_WORKSHOP = 2,
} Page;

static struct {
  Page page;
  GameRegistry* r;
  Layout* layout;
  Rectangle modal;
  Textbox tb;
  Listbox lb;
  Btn btn_choose;
  Btn btn_close;
  int sel;
  LevelDef** levels;

  // Label lab_name;
  Label lab_author;
  Btn btn_level_folder;
  Btn btn_local_folder;

  Btn btn_publish;
  Btn btn_wiki;
  Btn btn_open_steam;
  Btn btn_unsubscribe;
  Btn btn_browse_levels;

  Btn btn_page_workshop;
  Btn btn_page_official;
  Btn btn_page_local;
  int lb_hover;

  SolWidget sol;
} C = {0};

static void update_layout() {
  layout_update_offset(C.layout);
  Layout* l = C.layout;
  C.modal = layout_rect(l, "window");
  listbox_set_box(&C.lb, layout_rectb(l, "lb_level"));
  textbox_set_box(&C.tb, layout_rectb(l, "tb_desc"));
  C.btn_close.hitbox = layout_rectb(l, "btn_close");
  C.btn_choose.hitbox = layout_rectb(l, "btn_choose");
  C.btn_page_local.hitbox = layout_rectb(l, "btn_local");
  C.btn_page_workshop.hitbox = layout_rectb(l, "btn_workshop");
  C.btn_page_official.hitbox = layout_rectb(l, "btn_official");

  C.btn_publish.hitbox = layout_rectb(l, "btn_publish");
  C.btn_open_steam.hitbox = layout_rectb(l, "btn_open_steam");
  C.btn_unsubscribe.hitbox = layout_rectb(l, "btn_unsubscribe");
  C.btn_browse_levels.hitbox = layout_rectb(l, "btn_browse_levels");

  C.btn_level_folder.hitbox = layout_rectb(l, "btn_level_folder");
  C.btn_local_folder.hitbox = layout_rectb(l, "btn_local_folder");
  C.btn_wiki.hitbox = layout_rectb(l, "btn_wiki");

  C.lab_author.hitbox = layout_rectb(l, "lab_author");
  // C.lab_name.hitbox = layout_rect(l, "lab_name");
  sol_widget_update_layout(&C.sol, l);
}

static LevelDef** get_page_original_levels() {
  switch (C.page) {
    case PAGE_OFFICIAL:
      return C.r->official_custom_levels;
    case PAGE_WORKSHOP:
      return C.r->workshop_custom_levels;
    case PAGE_LOCAL:
      return C.r->local_custom_levels;
  }
}

static LevelDef* get_level(int s) { return C.levels[s]; }

static LevelDef* get_selected_level() {
  if (C.sel == -1) return NULL;
  return get_level(C.sel);
}

static int compare_level(const void* a, const void* b) {
  const LevelDef* const* la = a;
  const LevelDef* const* lb = b;
  return strcmp((*la)->name, (*lb)->name);
}

static void rebuild_listbox_items() {
  C.sel = -1;
  arrsetlen(C.levels, 0);
  LevelDef** orig_levels = get_page_original_levels();
  int no = arrlen(orig_levels);
  for (int i = 0; i < no; i++) {
    LevelDef* l = orig_levels[i];
    if (l->unsubscribed) continue;
    arrput(C.levels, l);
  }
  int n = arrlen(C.levels);
  /* Sorts the levels by name*/
  qsort(C.levels, n, sizeof(LevelDef*), compare_level);

  listbox_clear(&C.lb);
  // LevelDef* levels = get_page_levels();
  for (int i = 0; i < n; i++) {
    listbox_add_row(&C.lb, C.levels[i]->name);
  }
}

static void set_sel(int s) {
  if (s == -1) {
    textbox_set_content(&C.tb, "", NULL);
    label_set_text(&C.lab_author, "");
    // label_set_text(&C.lab_name, "");
    sol_widget_set_level_id(&C.sol, NULL);
  } else {
    LevelDef* ldef = get_level(s);
    textbox_set_content(&C.tb, ldef->description, ldef->sprites);
    // label_set_text(&C.lab_name, ldef->name);
    label_set_text(&C.lab_author, "");
    sol_widget_set_level_id(&C.sol, ldef->id);
  }
  C.sel = s;
}

static void select_first_item() {
  if (arrlen(C.levels) == 0) {
    set_sel(-1);
  } else {
    set_sel(0);
  }
}

static void set_page(Page page) {
  C.page = page;
  rebuild_listbox_items();
  select_first_item();
}

void win_customlvl_init() {
  C.r = getreg();
  C.layout = easy_load_layout("customlevel");
  sol_widget_init(&C.sol);
  textbox_init(&C.tb);
  listbox_init(&C.lb);
  C.lb.row_pad = 2;
  update_layout();
  set_page(0);
}

static void load_page_for_level(LevelDef* ldef) {
  if (ldef->is_custom_local) {
    set_page(PAGE_LOCAL);
  } else if (ldef->is_custom_steam) {
    set_page(PAGE_WORKSHOP);
  } else if (ldef->is_custom_official) {
    set_page(PAGE_OFFICIAL);
  } else {
    set_page(PAGE_OFFICIAL);
  }

  int n = arrlen(C.levels);
  for (int i = 0; i < n; i++) {
    if (strcmp(C.levels[i]->id, ldef->id) == 0) {
      set_sel(i);
      return;
    }
  }
  if (n > 0) {
    set_sel(0);
  } else {
    set_sel(-1);
  }
}

void win_customlvl_open(LevelDef* ldef) {
  ui_winpush(WINDOW_CUSTOM_LEVEL);
  update_layout();
  load_page_for_level(ldef);
}

static void update_page_buttons() {
  C.btn_page_workshop.disabled = !is_steam_on();
  if (btn_update(&C.btn_wiki)) {
    win_wiki_open_on_item("customlevel");
  }
  if (btn_update(&C.btn_page_local)) {
    set_page(PAGE_LOCAL);
  }
  if (btn_update(&C.btn_page_official)) {
    set_page(PAGE_OFFICIAL);
  }
  if (btn_update(&C.btn_local_folder)) {
    char* f = get_custom_levels_folder();
    open_file_explorer(f);
    free(f);
  }
  if (btn_update(&C.btn_page_workshop)) {
    set_page(PAGE_WORKSHOP);
  }
  C.btn_browse_levels.disabled = !is_steam_on();
  if (btn_update(&C.btn_browse_levels)) {
    steam_browse_workshop_levels();
  }
  C.btn_page_local.toggled = C.page == PAGE_LOCAL;
  C.btn_page_official.toggled = C.page == PAGE_OFFICIAL;
  C.btn_page_workshop.toggled = C.page == PAGE_WORKSHOP;

  C.btn_choose.disabled = C.sel == -1;
}

static void update_listbox() {
  listbox_update(&C.lb);
  // Listbox rows
  int hit = C.lb.row_hit;
  if (hit >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && C.sel != hit) {
    set_sel(hit);
  }
}

#if 0
static u64 extract_item_from_id(const char* id) {
  const char* colon = strchr(id, ':');
  if (colon) {
    u64 out = 0;
    if (sscanf(colon + 1, "%" PRIu64, &out) == 1) {
      return out;
    }
  }
  return 0;
}
#endif

static void publish_level() {
  LevelDef* lvl = get_selected_level();
  char* thumb_path = get_asset_path("imgs/default_level_thumbnail.png");
  char* thumb_path_abs = abs_path(thumb_path);
  free(thumb_path);
  PubformParams p = {0};
  char* folder = abs_path(lvl->folder);
  p.type = STEAM_ITEM_BLUEPRINT;
  char* tags[2] = {"level", NULL};
  p.num_tags = 1;
  p.tags = (const char**)tags;
  p.folder = folder;
  p.default_title = lvl->name ? lvl->name : "My Level";
  p.default_desc = lvl->description ? lvl->description : "";
  p.default_thumbnail_path = thumb_path_abs;

  win_pubform_open(p);
  free(thumb_path_abs);
  free(folder);
}

static void on_confirm_unsubscribe(int r) {
  if (r != 0) return;
  LevelDef* lvl = get_selected_level();
  u64 item = extract_item_from_id(lvl->id);
  steam_unsubscribe_item(item);
  lvl->unsubscribed = true;
  rebuild_listbox_items();
  select_first_item();
}

static void update_level_buttons() {
  C.btn_open_steam.disabled = (C.page != PAGE_WORKSHOP) || C.sel == -1;
  C.btn_unsubscribe.disabled = (C.page != PAGE_WORKSHOP) || C.sel == -1;
  C.btn_publish.disabled = (C.page != PAGE_LOCAL) || C.sel == -1;
  C.btn_level_folder.disabled = C.sel == -1;

  if (btn_update(&C.btn_publish)) {
    publish_level();
  }

  if (btn_update(&C.btn_unsubscribe)) {
    LevelDef* lvl = get_selected_level();
    dialog_open(TextFormat(T.customlvl_unsubscribe_confirm, lvl->name),
                T.unsubscribe, T.cancel, NULL, on_confirm_unsubscribe);
  }
  if (btn_update(&C.btn_open_steam)) {
    u64 item = extract_item_from_id(get_selected_level()->id);
    steam_open_overlay_item(item);
  }
  if (btn_update(&C.btn_level_folder)) {
    LevelDef* lvl = get_selected_level();
    open_file_explorer(lvl->folder);
  }
}

void win_customlvl_update() {
  update_layout();
  bool escape = IsKeyPressed(KEY_ESCAPE);
  if (btn_update(&C.btn_close) || escape) {
    ui_winpop();
    return;
  }
  if (btn_update(&C.btn_choose)) {
    ui_winpop();
    win_main_load_level(get_level(C.sel));
    return;
  }
  update_listbox();
  textbox_update(&C.tb);
  update_page_buttons();
  update_level_buttons();
  sol_widget_update(&C.sol);
}

static const char* make_win_title() {
  LevelDef* lvl = get_selected_level();
  if (!lvl) return T.customlvl_title;
  return lvl->name;
}

void win_customlvl_draw() {
  draw_win(C.modal, make_win_title());
  textbox_draw(&C.tb);
  listbox_draw(&C.lb, C.sel);
  C.btn_choose.primary = true;
  btn_draw_text(&C.btn_choose, T.levels_submit);
  btn_draw_text(&C.btn_close, T.close);
  Color bg = {0, 0, 0, 150};
  DrawRectangleRec(C.lab_author.hitbox, bg);

  DrawRectangleRec(C.btn_publish.hitbox, bg);
  DrawRectangleRec(C.btn_unsubscribe.hitbox, bg);
  DrawRectangleRec(C.btn_open_steam.hitbox, bg);
  DrawRectangleRec(C.btn_browse_levels.hitbox, bg);

  btn_draw_text(&C.btn_browse_levels, T.customlvl_workshop_btn);

  btn_draw_icon(&C.btn_publish, rect_publish);
  btn_draw_icon(&C.btn_open_steam, rect_steam);
  btn_draw_icon(&C.btn_unsubscribe, rect_trash);
  btn_draw_icon(&C.btn_wiki, rect_book);

  label_draw(&C.lab_author);
  // label_draw(&C.lab_name);

  btn_draw_text(&C.btn_page_workshop, T.customlvl_subscribed);
  btn_draw_text(&C.btn_page_official, T.customlvl_official);
  btn_draw_text(&C.btn_page_local, T.customlvl_local);

  btn_draw_icon(&C.btn_local_folder, rect_open);
  btn_draw_icon(&C.btn_level_folder, rect_open);

  sol_widget_draw(&C.sol);

  if (ui_get_window() == WINDOW_CUSTOM_LEVEL) {
    sol_widget_draw_leg(&C.sol);
    btn_draw_legend(&C.btn_wiki, T.customlvl_wiki_leg);

    btn_draw_legend(&C.btn_page_official, T.customlvl_official_leg);
    btn_draw_legend(&C.btn_page_local, T.customlvl_local_leg);
    btn_draw_legend(&C.btn_browse_levels, T.customlvl_browse_leg);
    btn_draw_legend(&C.btn_open_steam, T.customlvl_open_steam_leg);
    btn_draw_legend(&C.btn_publish, T.customlvl_publish_leg);
    btn_draw_legend(&C.btn_page_workshop, T.customlvl_workshop_leg);
    btn_draw_legend(&C.btn_unsubscribe, T.unsubscribe);

    btn_draw_legend(&C.btn_local_folder, T.customlvl_localfolder_leg);
    btn_draw_legend(&C.btn_level_folder, T.customlvl_levelfolder_leg);

    int hit = C.lb.row_hit;
    if (hit >= 0) {
      // LevelDef* lvls =
      Btn b = {0};
      b.hover = true;
      b.hitbox = C.lb.rows[hit].hitbox_g;
      LevelDef* ldef = get_level(hit);
      if (ldef->steam_author) {
        /* shows steam author on hover for steam items*/
        btn_draw_legend(&b, TextFormat(T.by_author, ldef->steam_author));
      }
    }
  }
}

void notify_installed_steam_level(const char* folder, u64 item) {
  add_steam_level_from_folder(folder, item);
}

LevelDef* find_sandbox_custom_level() {
  int n = arrlen(C.r->official_custom_levels);
  for (int i = 0; i < n; i++) {
    LevelDef* l = C.r->official_custom_levels[i];
    if (strcmp(l->id, "official:sandbox") == 0) {
      return l;
    }
  }
  return C.r->official_custom_levels[0];
}
