#include "win_customlvl.h"

#include "errno.h"
#include "fs.h"
#include "game_registry.h"
#include "layout.h"
#include "paths.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "wdialog.h"
#include "win_pubform.h"
#include "wmain.h"

typedef enum {
  PAGE_OFFICIAL = 0,
  PAGE_LOCAL = 1,
  PAGE_WORKSHOP = 2,
} Page;

typedef struct {
  Btn sol[4];
  Btn btn_prv;
  Btn btn_nxt;
  Btn btn_browse;
} SolWidget;

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
  CustomLevelDef** levels;

  // Label lab_name;
  Label lab_author;
  Btn btn_level_folder;
  Btn btn_local_folder;

  Btn btn_publish;
  Btn btn_open_steam;
  Btn btn_unsubscribe;
  Btn btn_browse_levels;

  Btn btn_file;
  Btn btn_page_workshop;
  Btn btn_page_official;
  Btn btn_page_local;
  int lb_hover;

  SolWidget sol;
} C = {0};

void sol_update_layout(SolWidget* s, Layout* l) {
  s->btn_prv.hitbox = layout_rect(l, "btn_sol_prv");
  s->btn_nxt.hitbox = layout_rect(l, "btn_sol_nxt");
  s->btn_browse.hitbox = layout_rect(l, "btn_browse_sol");
  s->sol[0].hitbox = layout_rect(l, "sol1");
  s->sol[1].hitbox = layout_rect(l, "sol2");
  s->sol[2].hitbox = layout_rect(l, "sol3");
  s->sol[3].hitbox = layout_rect(l, "sol4");
}

void sol_draw(SolWidget* s) {
  Color bg = {0, 0, 0, 150};
  DrawRectangleRec(s->sol[0].hitbox, bg);
  DrawRectangleRec(s->sol[1].hitbox, bg);
  DrawRectangleRec(s->sol[2].hitbox, bg);
  DrawRectangleRec(s->sol[3].hitbox, bg);
  DrawRectangleRec(s->btn_nxt.hitbox, bg);
  DrawRectangleRec(s->btn_prv.hitbox, bg);
  DrawRectangleRec(s->btn_browse.hitbox, bg);
}

void sol_draw_leg(SolWidget* s) {
  // TODO
}

void sol_update(SolWidget* s) {
  // TODO
}

static void update_layout() {
  layout_update_offset(C.layout);
  Layout* l = C.layout;
  C.modal = layout_rect(l, "window");
  listbox_set_box(&C.lb, layout_rect(l, "lb_level"));
  textbox_set_box(&C.tb, layout_rect(l, "tb_desc"));
  C.btn_close.hitbox = layout_rect(l, "btn_close");
  C.btn_choose.hitbox = layout_rect(l, "btn_choose");
  C.btn_page_local.hitbox = layout_rect(l, "btn_local");
  C.btn_page_workshop.hitbox = layout_rect(l, "btn_workshop");
  C.btn_page_official.hitbox = layout_rect(l, "btn_official");
  C.btn_file.hitbox = layout_rect(l, "btn_file");

  C.btn_publish.hitbox = layout_rect(l, "btn_publish");
  C.btn_open_steam.hitbox = layout_rect(l, "btn_open_steam");
  C.btn_unsubscribe.hitbox = layout_rect(l, "btn_unsubscribe");
  C.btn_browse_levels.hitbox = layout_rect(l, "btn_browse_levels");

  C.btn_level_folder.hitbox = layout_rect(l, "btn_level_folder");
  C.btn_local_folder.hitbox = layout_rect(l, "btn_local_folder");

  C.lab_author.hitbox = layout_rect(l, "lab_author");
  // C.lab_name.hitbox = layout_rect(l, "lab_name");
  sol_update_layout(&C.sol, l);
}

static CustomLevelDef** get_page_original_levels() {
  switch (C.page) {
    case PAGE_OFFICIAL:
      return C.r->official_custom_levels;
    case PAGE_WORKSHOP:
      return C.r->workshop_custom_levels;
    case PAGE_LOCAL:
      return C.r->local_custom_levels;
  }
}

static CustomLevelDef* get_level(int s) { return C.levels[s]; }

static CustomLevelDef* get_selected_level() {
  if (C.sel == -1) return NULL;
  return get_level(C.sel);
}

static int compare_level(const void* a, const void* b) {
  const CustomLevelDef* const* la = a;
  const CustomLevelDef* const* lb = b;
  return strcmp((*la)->name, (*lb)->name);
}

static void rebuild_listbox_items() {
  C.sel = -1;
  arrsetlen(C.levels, 0);
  CustomLevelDef** orig_levels = get_page_original_levels();
  int no = arrlen(orig_levels);
  for (int i = 0; i < no; i++) {
    CustomLevelDef* l = orig_levels[i];
    if (l->unsubscribed) continue;
    arrput(C.levels, l);
  }
  int n = arrlen(C.levels);
  /* Sorts the levels by name*/
  qsort(C.levels, n, sizeof(CustomLevelDef*), compare_level);

  listbox_clear(&C.lb);
  // CustomLevelDef* levels = get_page_levels();
  for (int i = 0; i < n; i++) {
    listbox_add_row(&C.lb, C.levels[i]->name);
  }
}

static void set_sel(int s) {
  if (s == -1) {
    textbox_set_content(&C.tb, "", NULL);
    label_set_text(&C.lab_author, "");
    // label_set_text(&C.lab_name, "");
  } else {
    CustomLevelDef* ldef = get_level(s);
    textbox_set_content(&C.tb, ldef->desc, ldef->desc_imgs);
    // label_set_text(&C.lab_name, ldef->name);
    label_set_text(&C.lab_author, "");
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

void win_customlvl_init(GameRegistry* r) {
  C.r = r;
  C.layout = easy_load_layout("customlevel");
  textbox_init(&C.tb);
  listbox_init(&C.lb);
  C.lb.row_pad = 2;
  update_layout();
  set_page(0);
}

static void load_page_for_level(CustomLevelDef* ldef) {
  switch (ldef->type) {
    case CUSTOM_LEVEL_LOCAL:
      set_page(PAGE_LOCAL);
      break;
    case CUSTOM_LEVEL_STEAM:
      set_page(PAGE_WORKSHOP);
      break;
    case CUSTOM_LEVEL_OFFICIAL:
      set_page(PAGE_OFFICIAL);
      break;
    case CUSTOM_LEVEL_UNK:
      abort();
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

void win_customlvl_open(CustomLevelDef* ldef) {
  ui_winpush(WINDOW_CUSTOM_LEVEL);
  update_layout();
  if (!ldef) {
    set_page(0);
  } else {
    load_page_for_level(ldef);
  }
}
static void update_page_buttons() {
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
  if (btn_update(&C.btn_file)) {
    bool loaded = custom_level_open_file();
    if (loaded) {
      ui_winpop();
      return;
    }
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
  CustomLevelDef* lvl = get_selected_level();
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
  p.default_desc = lvl->desc ? lvl->desc : "";
  p.default_thumbnail_path = thumb_path_abs;

  win_pubform_open(p);
  free(thumb_path_abs);
  free(folder);
}

static void on_confirm_unsubscribe(int r) {
  if (r != 0) return;
  CustomLevelDef* lvl = get_selected_level();
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
    CustomLevelDef* lvl = get_selected_level();
    dialog_open(TextFormat("Unsubscribe from level \"%s\"?", lvl->name),
                "Unsubscribe", "Cancel", NULL, on_confirm_unsubscribe);
  }
  if (btn_update(&C.btn_open_steam)) {
    u64 item = extract_item_from_id(get_selected_level()->id);
    steam_open_overlay_item(item);
  }
  if (btn_update(&C.btn_level_folder)) {
    CustomLevelDef* lvl = get_selected_level();
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
    main_load_custom_level(get_level(C.sel));
    return;
  }
  update_listbox();
  textbox_update(&C.tb);
  update_page_buttons();
  update_level_buttons();
  sol_update(&C.sol);
}

static const char* get_level_prefix(CustomLevelDef* lvl) {
  switch (lvl->type) {
    case CUSTOM_LEVEL_LOCAL:
      return "[local]";
    case CUSTOM_LEVEL_STEAM:
      return "[workshop]";
    case CUSTOM_LEVEL_OFFICIAL:
      return "[official]";
    case CUSTOM_LEVEL_UNK:
      return "[???]";
  }
}

static const char* make_win_title() {
  CustomLevelDef* lvl = get_selected_level();
  if (!lvl) return "CUSTOM LEVELS";
  return lvl->name;
}

void win_customlvl_draw() {
  draw_win(C.modal, make_win_title());
  textbox_draw(&C.tb);
  listbox_draw(&C.lb, C.sel);
  btn_draw_text_primary(&C.btn_choose, ui_get_scale(), "SELECT LEVEL");
  btn_draw_text(&C.btn_close, ui_get_scale(), "CLOSE");
  Color bg = {0, 0, 0, 150};
  DrawRectangleRec(C.lab_author.hitbox, bg);

  DrawRectangleRec(C.btn_publish.hitbox, bg);
  DrawRectangleRec(C.btn_unsubscribe.hitbox, bg);
  DrawRectangleRec(C.btn_open_steam.hitbox, bg);
  DrawRectangleRec(C.btn_browse_levels.hitbox, bg);

  btn_draw_text(&C.btn_browse_levels, ui_get_scale(), "WORKSHOP");

  Texture sprites = ui_get_sprites();
  btn_draw_icon(&C.btn_publish, ui_get_scale(), sprites, rect_publish);
  btn_draw_icon(&C.btn_open_steam, ui_get_scale(), sprites, rect_steam);
  btn_draw_icon(&C.btn_unsubscribe, ui_get_scale(), sprites, rect_trash);

  label_draw(&C.lab_author);
  // label_draw(&C.lab_name);

  btn_draw_text(&C.btn_page_workshop, ui_get_scale(), "Subscribed");
  btn_draw_text(&C.btn_page_official, ui_get_scale(), "Official");
  btn_draw_text(&C.btn_page_local, ui_get_scale(), "Local");
  btn_draw_icon(&C.btn_file, ui_get_scale(), sprites, rect_lua);

  btn_draw_icon(&C.btn_local_folder, ui_get_scale(), sprites, rect_open);
  btn_draw_icon(&C.btn_level_folder, ui_get_scale(), sprites, rect_open);
  // sol_draw(&C.sol);

  if (ui_get_window() == WINDOW_CUSTOM_LEVEL) {
    //     sol_draw_leg(&C.sol);

    btn_draw_legend(&C.btn_page_official, ui_get_scale(),
                    "Official custom levels of the game");
    btn_draw_legend(&C.btn_page_local, ui_get_scale(),
                    "Local custom levels located in the local_levels/ "
                    "folder.\nThese levels "
                    "can be published to Steam Workshop.");
    btn_draw_legend(&C.btn_browse_levels, ui_get_scale(),
                    "Browse community levels in Steam Workshop");
    btn_draw_legend(&C.btn_open_steam, ui_get_scale(),
                    "See item's page on workshop");
    btn_draw_legend(&C.btn_publish, ui_get_scale(),
                    "Publish level on steam workshop");
    btn_draw_legend(&C.btn_page_workshop, ui_get_scale(),
                    "Levels subscribed on Steam Workshop");
    btn_draw_legend(&C.btn_unsubscribe, ui_get_scale(), "Unsubscribe");
    btn_draw_legend(&C.btn_file, ui_get_scale(),
                    "Select custom level from local files");

    btn_draw_legend(&C.btn_local_folder, ui_get_scale(),
                    "Open folder containing your local custom levels");
    btn_draw_legend(&C.btn_level_folder, ui_get_scale(),
                    "Open folder containing this level's content");

    int hit = C.lb.row_hit;
    if (hit >= 0) {
      // CustomLevelDef* lvls =
      Btn b = {0};
      b.hover = true;
      b.hitbox = C.lb.rows[hit].hitbox_g;
      CustomLevelDef* ldef = get_level(hit);
      if (ldef->steam_author) {
        /* shows steam author on hover for steam items*/
        btn_draw_legend(&b, 2, TextFormat("by %s", ldef->steam_author));
      }
    }
  }
}

void notify_installed_steam_level(const char* folder, u64 item) {
  add_steam_level_from_folder(C.r, folder, item);
}
