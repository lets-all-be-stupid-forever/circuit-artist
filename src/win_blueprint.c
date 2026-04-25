#include "win_blueprint.h"

#include "assert.h"
#include "font.h"
#include "img.h"
#include "json.h"
#include "msg.h"
#include "paths.h"
#include "sound.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ui.h"
#include "utils.h"
#include "wdialog.h"
#include "widgets.h"
#include "wmain.h"
#include "wtext.h"

#define WIN_BLUEPRINT_win_blueprint ((Rectangle){0, 0, 1168, 682})
#define WIN_BLUEPRINT_btn_close ((Rectangle){1006, 631, 132, 32})
#define WIN_BLUEPRINT_btn_delete ((Rectangle){28, 631, 116, 32})
#define WIN_BLUEPRINT_btn_rename ((Rectangle){153, 631, 116, 32})
#define WIN_BLUEPRINT_btn_use ((Rectangle){404, 631, 322, 32})
#define WIN_BLUEPRINT_staging ((Rectangle){30, 168, 128, 448})
#define WIN_BLUEPRINT_pages ((Rectangle){180, 73, 960, 64})
#define WIN_BLUEPRINT_slots ((Rectangle){180, 168, 960, 448})
#define WIN_BLUEPRINT_unlock ((Rectangle){128, 73, 34, 34})
#define WIN_BLUEPRINT_btn_rot ((Rectangle){279, 631, 116, 32})
#define BLUEPRINT_FILE_1_1 "blueprints_1_1.json"
#define BLUEPRINT_FILE_v2 "blueprints_v2.json"

#define S_COLS 15
#define S_ROWS 7
#define F_COLS 2
#define NUM_PAGES S_COLS
#define NUM_FIXED (F_COLS * S_ROWS)
#define PAGESIZE (S_COLS * S_ROWS)
#define TOTAL_BLUEPRINTS (NUM_FIXED + NUM_PAGES * PAGESIZE + NUM_PAGES)

/* Blueprints are mapped as: (i) () Staging, (MxNinventory) (N pages) */
static struct {
  Blueprint* blueprints[TOTAL_BLUEPRINTS];
  Rectangle modal;
  Btn slots[PAGESIZE];
  Btn page_slots[NUM_PAGES];
  Btn fixed_slots[NUM_FIXED];
  Btn btn_close;
  Btn btn_rename;
  Btn btn_delete;
  Btn btn_use;
  Btn btn_rot;
  Btn btn_editpage;
  int sel;
  int apage; /* active page */
  bool editpage;
} C = {0};

#define LAYOUT(name) roff(off, WIN_BLUEPRINT_##name)

const char* get_blueprint_path() { return get_data_path(BLUEPRINT_FILE_v2); }

// blueprints_v2.json: positional array, each slot is null or {id, rot}
static void blueprint_save() {
  json_object* root = json_object_new_object();
  if (!root) {
    ui_crash("Couldn't create blueprint inventory file.");
    return;
  }
  json_object_object_add(root, "version", json_object_new_int(2));
  json_object* bps = json_object_new_array();
  for (int i = 0; i < TOTAL_BLUEPRINTS; i++) {
    Blueprint* s = C.blueprints[i];
    json_object* bp;
    if (!s) {
      bp = json_object_new_null();
    } else {
      bp = json_object_new_object();
      json_object_object_add(bp, "id", json_object_new_string(s->id));
      json_object_object_add(bp, "rot", json_object_new_int(s->rot));
    }
    json_object_array_add(bps, bp);
  }
  json_object_object_add(root, "blueprints", bps);
  if (json_object_to_file_ext(get_blueprint_path(), root,
                              JSON_C_TO_STRING_PRETTY) < 0) {
    fprintf(stderr, "Error writing blueprint inventory\n");
    ui_crash("Error writing blueprint inventory\n");
  }
  json_object_put(root);
}

static void blueprint_save_meta(Blueprint* s) {
  json_object* meta = json_object_new_object();
  json_object_object_add(meta, "id", json_object_new_string(s->id));
  json_object_object_add(meta, "name", json_object_new_string(s->name));
  json_object_to_file_ext(
      get_data_path(TextFormat("blueprints_v2/%s/meta.json", s->id)), meta,
      JSON_C_TO_STRING_PRETTY);
  json_object_put(meta);
}

const char* blueprint_fname_full(Blueprint* s) {
  return TextFormat("%s/full.png", s->folder);
}

const char* blueprint_fname_thumbnail(Blueprint* s) {
  return TextFormat("%s/thumb.png", s->folder);
}

static int get_page_slot_blueprint_idx(int page, int i) {
  int off = NUM_FIXED + page * PAGESIZE;
  return off + i;
}

static int get_slot_blueprint_idx(int i) {
  return get_page_slot_blueprint_idx(C.apage, i);
}

static int get_page_blueprint_idx(int i) {
  int off = NUM_FIXED + NUM_PAGES * PAGESIZE;
  return off + i;
}

static int get_fixed_blueprint_idx(int i) { return i; }

static int find_available_fixed_slot() {
  for (int i = 0; i < NUM_FIXED; i++) {
    int sidx = get_fixed_blueprint_idx(i);
    if (!C.blueprints[sidx]) return sidx;
  }
  return -1;
}

static int find_available_page_slot(int page) {
  for (int i = 0; i < PAGESIZE; i++) {
    int sidx = get_page_slot_blueprint_idx(page, i);
    if (!C.blueprints[sidx]) return sidx;
  }
  return -1;
}

static int find_first_available_slot() {
  int c = -1;
  c = find_available_fixed_slot();
  if (c >= 0) return c;
  c = find_available_page_slot(C.apage);
  if (c >= 0) return c;
  for (int i = 0; i < NUM_PAGES; i++) {
    c = find_available_page_slot(i);
    if (c >= 0) return c;
  }
  return -1;
}

static void blueprint_rm_files(Blueprint* bp) {
  if (!bp || !bp->folder) return;
  char path[1024];
  snprintf(path, sizeof(path), "%s/thumb.png", bp->folder);
  delete_file(path);
  snprintf(path, sizeof(path), "%s/full.png", bp->folder);
  delete_file(path);
  snprintf(path, sizeof(path), "%s/meta.json", bp->folder);
  delete_file(path);
  remove(bp->folder);
}

static void blueprint_destroy(Blueprint* bp) {
  if (!bp) return;
  free(bp->id);
  free(bp->name);
  free(bp->folder);
  UnloadTexture(bp->thumbnail);
  free(bp);
}

static int blueprint_create(int nl, Image* imgs, Image full) {
  int ibp = find_first_available_slot();
  if (ibp == -1) return -1;
  assert(!C.blueprints[ibp]);

  Image thumb = gen_thumbnail(nl, imgs, 64, 64);
  char* id = clone_string(randid());
  Blueprint* bp = calloc(1, sizeof(Blueprint));
  bp->id = id;
  bp->name = clone_string("");
  bp->folder = clone_string(get_data_path(TextFormat("blueprints_v2/%s", id)));
  C.blueprints[ibp] = bp;

  // Create per-blueprint folder and write files
  MakeDirectory(bp->folder);
  ExportImage(full, blueprint_fname_full(bp));
  ExportImage(thumb, blueprint_fname_thumbnail(bp));
  blueprint_save_meta(bp);

  bp->thumbnail = LoadTextureFromImage(thumb);
  UnloadImage(thumb);

  blueprint_save();
  return ibp;
}

static void swap_blueprints(int i0, int i1) {
  Blueprint* tmp = C.blueprints[i0];
  C.blueprints[i0] = C.blueprints[i1];
  C.blueprints[i1] = tmp;
}

static void add_bp_from_img_asset(const char* asset) {
  Image img = load_image_asset(asset);
  int nl = 0;
  Image tmp[MAX_LAYERS] = {0};
  image_decode_layers(img, &nl, tmp);
  blueprint_create(nl, tmp, img);
  for (int i = 0; i < nl; i++) {
    UnloadImage(tmp[i]);
  }
  UnloadImage(img);
}

static void initialize_bps() {
  add_bp_from_img_asset("default_blueprints/default_page_1.png");
  add_bp_from_img_asset("default_blueprints/default_page_2.png");
  add_bp_from_img_asset("default_blueprints/default_bp1.png");
  add_bp_from_img_asset("default_blueprints/default_bp2.png");
  add_bp_from_img_asset("default_blueprints/default_bp3.png");
  add_bp_from_img_asset("default_blueprints/default_bp4.png");
  swap_blueprints(0, get_page_blueprint_idx(0));
  swap_blueprints(1, get_page_blueprint_idx(1));
  swap_blueprints(2, get_page_slot_blueprint_idx(0, 0));
  swap_blueprints(3, get_page_slot_blueprint_idx(0, 1));
  swap_blueprints(4, get_page_slot_blueprint_idx(0, 2));
  swap_blueprints(5, get_page_slot_blueprint_idx(0, 3));
  blueprint_save();
}

// Loads a blueprint from a folder and places it in C.blueprints.
// If a stub (id-only Blueprint) already occupies a slot with the matching id,
// that slot is filled in. Otherwise the first free slot is used.
// Does nothing if the folder has no valid meta.json + thumb.png.
static void inject_blueprint_from_folder(const char* folder) {
  char meta_path[1024], thumb_path[1024];
  snprintf(meta_path, sizeof(meta_path), "%s/meta.json", folder);
  snprintf(thumb_path, sizeof(thumb_path), "%s/thumb.png", folder);
  if (!FileExists(meta_path) || !FileExists(thumb_path)) return;

  json_object* meta = json_object_from_file(meta_path);
  if (!meta) return;

  json_object *id_obj = NULL, *name_obj = NULL;
  json_object_object_get_ex(meta, "id", &id_obj);
  json_object_object_get_ex(meta, "name", &name_obj);
  if (!id_obj) {
    json_object_put(meta);
    return;
  }

  const char* id = json_object_get_string(id_obj);
  const char* name = name_obj ? json_object_get_string(name_obj) : "";

  // Find an existing stub slot with the matching id.
  int slot = -1;
  for (int i = 0; i < TOTAL_BLUEPRINTS; i++) {
    if (C.blueprints[i] && strcmp(C.blueprints[i]->id, id) == 0) {
      slot = i;
      break;
    }
  }

  if (slot == -1) {
    // No reserved slot — use any free slot.
    slot = find_first_available_slot();
    if (slot == -1) {
      json_object_put(meta);
      return;
    }
    Blueprint* bp = calloc(1, sizeof(Blueprint));
    bp->id = clone_string(id);
    bp->folder = clone_string(folder);
    C.blueprints[slot] = bp;
  }

  Blueprint* bp = C.blueprints[slot];
  free(bp->name);
  free(bp->folder);
  bp->name = clone_string(name);
  bp->folder = clone_string(folder);
  bp->thumbnail = LoadTexture(thumb_path);

  json_object_put(meta);
}

// Scans blueprints_v2/ for subfolders and calls inject_blueprint_from_folder
// on each. Call this to pick up locally stored blueprints.
static void register_local_blueprints() {
  char v2dir[512];
  snprintf(v2dir, sizeof(v2dir), "%s", get_data_path("blueprints_v2"));
  if (!DirectoryExists(v2dir)) return;

  FilePathList dirs = LoadDirectoryFiles(v2dir);
  for (unsigned int i = 0; i < dirs.count; i++) {
    if (DirectoryExists(dirs.paths[i])) {
      inject_blueprint_from_folder(dirs.paths[i]);
    }
  }
  UnloadDirectoryFiles(dirs);
}

void blueprint_load() {
  if (!FileExists(get_blueprint_path())) {
    initialize_bps();
    return;
  }

  // Step 1: Parse blueprints_v2.json and reserve id-only stubs at the
  // recorded slot positions (preserves slot layout even before folders
  // are scanned).
  {
    json_object* root = json_object_from_file(get_blueprint_path());
    if (root) {
      json_object* bps = NULL;
      json_object_object_get_ex(root, "blueprints", &bps);
      if (bps) {
        int n = (int)json_object_array_length(bps);
        if (n > TOTAL_BLUEPRINTS) n = TOTAL_BLUEPRINTS;
        for (int i = 0; i < n; i++) {
          json_object* bp = json_object_array_get_idx(bps, i);
          if (!bp || json_object_get_type(bp) == json_type_null) continue;
          json_object *id_obj = NULL, *rot_obj = NULL;
          json_object_object_get_ex(bp, "id", &id_obj);
          json_object_object_get_ex(bp, "rot", &rot_obj);
          if (!id_obj) continue;
          Blueprint* stub = calloc(1, sizeof(Blueprint));
          stub->id = clone_string(json_object_get_string(id_obj));
          stub->name = clone_string("");
          if (rot_obj) stub->rot = json_object_get_int(rot_obj);
          C.blueprints[i] = stub;
        }
      }
      json_object_put(root);
    }
  }

  // Step 2: Scan actual folders and fill in (or add) blueprints.
  register_local_blueprints();

  // Step 3: Remove stubs that were never matched to a real folder
  // (thumbnail.id == 0 means no texture was loaded).
  for (int i = 0; i < TOTAL_BLUEPRINTS; i++) {
    Blueprint* bp = C.blueprints[i];
    if (bp && bp->thumbnail.id == 0) {
      blueprint_destroy(bp);
      C.blueprints[i] = NULL;
    }
  }
}

static void fix_slot_layout(Btn* buttons, int x0, int y0, int rows, int cols) {
  int slotw = 64;
  int sloth = 64;
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      int i = y * cols + x;
      int xx = x0 + x * slotw;
      int yy = y0 + y * sloth;
      buttons[i].hitbox = (Rectangle){
          .x = xx,
          .y = yy,
          .width = slotw,
          .height = sloth,
      };
    }
  }
}

static void update_layout() {
  C.modal = WIN_BLUEPRINT_win_blueprint;
  Vector2 off = find_modal_off(C.modal);
  C.modal = LAYOUT(win_blueprint);
  C.btn_close.hitbox = LAYOUT(btn_close);
  C.btn_rename.hitbox = LAYOUT(btn_rename);
  C.btn_delete.hitbox = LAYOUT(btn_delete);
  C.btn_use.hitbox = LAYOUT(btn_use);
  C.btn_rot.hitbox = LAYOUT(btn_rot);
  C.btn_editpage.hitbox = LAYOUT(unlock);
  Rectangle slots = LAYOUT(slots);
  Rectangle staging = LAYOUT(staging);
  Rectangle pages = LAYOUT(pages);
  int rows = 7;
  int cols = 15;

  fix_slot_layout(C.slots, slots.x, slots.y, rows, cols);
  fix_slot_layout(C.fixed_slots, staging.x, staging.y, rows, 2);
  fix_slot_layout(C.page_slots, pages.x, pages.y, 1, cols);

  int pad = 0;
}

static void ensure_blueprints_folder_exists() {
  if (!DirectoryExists(get_data_path("blueprints_v2"))) {
    MakeDirectory(get_data_path("blueprints_v2"));
  }
}

static void copy_file_if_exists(const char* src, const char* dst) {
  if (!FileExists(src)) return;
  int size = 0;
  unsigned char* data = LoadFileData(src, &size);
  if (data) {
    SaveFileData(dst, data, size);
    UnloadFileData(data);
  }
}

// Migrates v1 blueprint data (blueprints_1_1.json + blueprints/<id>_*.png)
// to v2 layout (blueprints_v2/<id>/{full.png,thumb.png,meta.json} +
// blueprints_v2.json). Does NOT modify or delete v1 files. Safe to call
// multiple times (skips if v2 already exists).
static void migrate_v1_to_v2() {
  if (!FileExists(get_data_path(BLUEPRINT_FILE_1_1))) return;
  if (FileExists(get_data_path(BLUEPRINT_FILE_v2))) return;

  json_object* root_v1 =
      json_object_from_file(get_data_path(BLUEPRINT_FILE_1_1));
  if (!root_v1) return;

  json_object* stamps_v1 = NULL;
  if (!json_object_object_get_ex(root_v1, "stamps", &stamps_v1)) {
    json_object_put(root_v1);
    return;
  }

  int count = json_object_array_length(stamps_v1);

  json_object* root_v2 = json_object_new_object();
  json_object_object_add(root_v2, "version", json_object_new_int(2));
  json_object* bps_v2 = json_object_new_array();

  for (int i = 0; i < count; i++) {
    json_object* entry = json_object_array_get_idx(stamps_v1, i);
    if (!entry || json_object_get_type(entry) == json_type_null) {
      json_object_array_add(bps_v2, json_object_new_null());
      continue;
    }

    json_object *id_obj, *name_obj, *rot_obj;
    if (!json_object_object_get_ex(entry, "id", &id_obj) ||
        !json_object_object_get_ex(entry, "name", &name_obj)) {
      json_object_array_add(bps_v2, json_object_new_null());
      continue;
    }

    const char* id = json_object_get_string(id_obj);
    const char* name = json_object_get_string(name_obj);
    int rot = 0;
    if (json_object_object_get_ex(entry, "rot", &rot_obj))
      rot = json_object_get_int(rot_obj);

    // Create per-blueprint folder (MakeDirectory creates intermediaries)
    MakeDirectory(get_data_path(TextFormat("blueprints_v2/%s", id)));

    // Copy images from v1 location to v2 location
    copy_file_if_exists(
        get_data_path(TextFormat("blueprints/%s_full.png", id)),
        get_data_path(TextFormat("blueprints_v2/%s/full.png", id)));
    copy_file_if_exists(
        get_data_path(TextFormat("blueprints/%s_thumb.png", id)),
        get_data_path(TextFormat("blueprints_v2/%s/thumb.png", id)));

    // Write meta.json
    json_object* meta = json_object_new_object();
    json_object_object_add(meta, "id", json_object_new_string(id));
    json_object_object_add(meta, "name", json_object_new_string(name));
    json_object_to_file_ext(
        get_data_path(TextFormat("blueprints_v2/%s/meta.json", id)), meta,
        JSON_C_TO_STRING_PRETTY);
    json_object_put(meta);

    // Add minimal entry to v2 master (id + rot only; name lives in meta.json)
    json_object* bp_v2 = json_object_new_object();
    json_object_object_add(bp_v2, "id", json_object_new_string(id));
    json_object_object_add(bp_v2, "rot", json_object_new_int(rot));
    json_object_array_add(bps_v2, bp_v2);
  }

  json_object_object_add(root_v2, "blueprints", bps_v2);
  json_object_to_file_ext(get_data_path(BLUEPRINT_FILE_v2), root_v2,
                          JSON_C_TO_STRING_PRETTY);
  json_object_put(root_v2);
  json_object_put(root_v1);
}

void win_blueprint_init() {
  migrate_v1_to_v2();
  ensure_blueprints_folder_exists();
  blueprint_load();
}

void win_blueprint_open() {
  ui_winpush(WINDOW_BLUEPRINT);
  update_layout();
  C.sel = -1;
}

static void on_confirm_delete(int r) {
  if (r == 1 || r == -1) return;
  int i = C.sel;
  if (i != -1 && C.blueprints[i]) {
    Blueprint* s = C.blueprints[i];
    blueprint_rm_files(s);
    blueprint_destroy(C.blueprints[i]);
    C.blueprints[i] = NULL;
    blueprint_save();
    msg_add("Blueprint deleted.", 4);
    C.sel = -1;
  }
}

static void set_page(int page) { C.apage = page; }

static void gotopage_of(int ibp) {
  if (ibp < NUM_FIXED) return;
  int p = (ibp - NUM_FIXED) / PAGESIZE;
  set_page(p);
}

static void on_rename_accept(void* ctx, const char* txt) {
  int i = C.sel;
  if (i != -1 && C.blueprints[i]) {
    Blueprint* s = C.blueprints[i];
    free(s->name);
    s->name = clone_string(txt);
    blueprint_save_meta(s);
    C.sel = -1;
  }
}

static void use_blueprint(int ibp) {
  Blueprint* s = C.blueprints[ibp];
  assert(s);
  main_paste_file(blueprint_fname_full(s), s->rot);
  play_sound_click();
  ui_winpop();
  blueprint_save();
  return;
}

static void update_slot(Btn* b, Vector2 mouse, int sidx) {
  b->hover = rect_hover(b->hitbox, mouse);
  bool isfixed = sidx < NUM_FIXED;
  if (b->hover) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      if (C.blueprints[sidx]) {
        use_blueprint(sidx);
      }
    }
    if (C.sel != -1 && sidx != C.sel) {
      ui_set_cursor(MOUSE_SWITCH);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      bool empty = C.blueprints[sidx] == NULL;
      if (C.sel != -1) {
        if (sidx == C.sel) {
          // use_sel();
          C.sel = -1;
          play_sound_click();
          return;
        } else {
          swap_blueprints(C.sel, sidx);
          C.sel = -1;
          play_sound_click();
        }
      } else {
        if (C.blueprints[sidx]) {
          play_sound_click();
          if (is_control_down()) {
            /* Tries to move to fixed slot*/
            if (!isfixed) {
              int tgt = find_available_fixed_slot();
              if (tgt >= 0) {
                swap_blueprints(sidx, tgt);
              }
            } else {
              int tgt = find_available_page_slot(C.apage);
              if (tgt >= 0) {
                swap_blueprints(sidx, tgt);
              }
            }
          } else {
            C.sel = sidx;
          }
        }
      }
    }
  }
}

static void update_page_slot(Btn* b, Vector2 mouse, int sidx) {
  b->hover = rect_hover(b->hitbox, mouse);
  if (C.editpage) {
    update_slot(b, mouse, sidx);
  } else {
    if (b->hover) {
      ui_set_cursor(MOUSE_POINTER);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        play_sound_click();
        int ipage = sidx - NUM_FIXED - NUM_PAGES * PAGESIZE;
        set_page(ipage);
      }
    }
  }
}
static void on_del_blueprint() {
  Blueprint* s = C.blueprints[C.sel];
  dialog_open(TextFormat("Delete blueprint %s ?", s->name), "Delete", "Cancel",
              NULL, on_confirm_delete);
  assert(s);
}

static void rot_blueprint() {
  C.blueprints[C.sel]->rot = (C.blueprints[C.sel]->rot + 1) % 4;
}

void win_blueprint_update() {
  update_layout();
  bool key_escape = IsKeyPressed(KEY_ESCAPE);
  bool key_del = IsKeyPressed(KEY_DELETE);
  bool key_backspace = IsKeyPressed(KEY_BACKSPACE);
  bool key_q = IsKeyPressed(KEY_Q);
  bool key_r = IsKeyPressed(KEY_R);
  if (btn_update(&C.btn_close) || (key_escape && C.sel == -1) || key_q) {
    ui_winpop();
    blueprint_save();
    return;
  }

  if (key_r && C.sel != -1) {
    rot_blueprint();
    return;
  }

  if (btn_update(&C.btn_rot)) {
    rot_blueprint();
    return;
  }

  /* Escape de-selects blueprint */
  if (key_escape && C.sel != -1) {
    C.sel = -1;
  }

  C.btn_use.disabled = C.sel == -1;
  C.btn_rot.disabled = C.sel == -1;
  C.btn_delete.disabled = C.sel == -1;
  C.btn_rename.disabled = C.sel == -1;

  if (btn_update(&C.btn_editpage)) {
    C.editpage = !C.editpage;
    if (C.sel >= NUM_FIXED + NUM_PAGES * PAGESIZE) {
      C.sel = -1;
    }
    return;
  }
  C.btn_editpage.toggled = C.editpage;

  if (btn_update(&C.btn_rename)) {
    text_modal_open(on_rename_accept, NULL, C.blueprints[C.sel]->name);
    return;
  }

  if (btn_update(&C.btn_use)) {
    use_blueprint(C.sel);
    return;
  }

  if (btn_update(&C.btn_delete)) {
    on_del_blueprint();
    return;
  }
  if (C.sel != -1 && (key_del || key_backspace)) {
    on_del_blueprint();
    return;
  }

  Vector2 mouse = GetMousePosition();
  for (int i = 0; i < PAGESIZE; i++) {
    update_slot(&C.slots[i], mouse, get_slot_blueprint_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    update_slot(&C.fixed_slots[i], mouse, get_fixed_blueprint_idx(i));
  }
  for (int i = 0; i < NUM_PAGES; i++) {
    update_page_slot(&C.page_slots[i], mouse, get_page_blueprint_idx(i));
  }
}

static void draw_sel_item() {
  if (C.sel == -1) return;
  Vector2 mouse = GetMousePosition();

  rlPushMatrix();
  rlTranslatef(mouse.x, mouse.y + 15, 0);
  Blueprint* s = C.blueprints[C.sel];
  Color c = WHITE;
  c.a = 200;
  DrawTexture(s->thumbnail, 0, 0, c);
  rlPopMatrix();
}

static void draw_t(const char* t, Rectangle r) {
  rlPushMatrix();
  rlTranslatef(r.x, r.y - 24, 0);
  rlScalef(2, 2, 1);
  font_draw_texture(t, 0, 0, WHITE);
  rlPopMatrix();
}

static void draw_labels() {
  draw_t(C.editpage ? "Pages (editing)" : "Pages", C.page_slots[0].hitbox);
  draw_t("Inventory", C.slots[0].hitbox);
  draw_t("Fixed Slots", C.fixed_slots[0].hitbox);
}

static void draw_slot(Btn* b, int sidx) {
  Blueprint* s = C.blueprints[sidx];
  Color c = BLACK;
  c.a = 128;
  Rectangle slot = b->hitbox;
  bool hover = b->hover;
  bool empty = (s == NULL);
  if (s) {
    DrawRectangleRec(slot, BLACK);
  } else {
    DrawRectangleRec(slot, c);
  }
  if (s) {
    int x = slot.x;
    int y = slot.y;
    int tw = slot.width;
    int th = slot.height;  // slot.height;
    int sw = s->thumbnail.width;
    int sh = s->thumbnail.height;
    int xx = x + (tw - sw) / 2;
    int yy = y + (th - sh) / 2;
    float hh = th / 2.0;
    float ww = tw / 2.0;
    rlPushMatrix();
    rlTranslatef(xx, yy, 0);
    // DrawTexture(s->thumbnail, 0, 0, WHITE);
    Rectangle src = {0, 0, sw, sh};
    Rectangle dst = {0, 0, sw, sh};
    rlTranslatef(sw / 2, sh / 2, 0);
    DrawTexturePro(s->thumbnail, src, dst, (Vector2){sw / 2, sh / 2},
                   s->rot * 90, WHITE);
    rlPopMatrix();
  }
}

static void draw_slot_hover(Btn* b, int sidx) {
  bool hover = b->hover;
  Blueprint* s = C.blueprints[sidx];
  if (hover) {
    Rectangle r = b->hitbox;
    Rectangle r2 = {r.x - 2, r.y - 2, r.width + 4, r.height + 4};
    Color c = WHITE;
    c.a = 100;
    DrawRectangleLinesEx(r2, 2, c);
  }
}

static void draw_slot_sel(Btn* b, int sidx) {
  bool selected = sidx == C.sel;
  Rectangle r = b->hitbox;
  if (selected) {
    Color k = GetColor(0xBB830BFF);
    rlPushMatrix();
    Rectangle r2 = {r.x - 4, r.y - 4, r.width + 8, r.height + 8};
    DrawRectangleLinesEx(r2, 4, k);
    rlPopMatrix();
  }
}

static void draw_slot_hover_leg(Btn* b, int sidx) {
  bool hover = b->hover;
  Blueprint* s = C.blueprints[sidx];
  if (s && hover) {
    const char* hover_name = s->name;
    if (strlen(hover_name) == 0) return;
    Rectangle r = b->hitbox;
    rlPushMatrix();
    rlTranslatef(r.x, r.y - 24, 0);
    rlScalef(2, 2, 1);
    int w = get_rendered_text_size(hover_name).x;
    Color c = BLACK;
    c.a = 150;
    DrawRectangle(0, -2, w, 12 + 4, c);
    font_draw_texture_outlined(hover_name, 0, 0, WHITE, BLACK);
    rlPopMatrix();
  }
}

void win_blueprint_draw() {
  draw_win(C.modal, "BLUEPRINTS");

  Color k = GetColor(0xBB830BFF);
  draw_labels();

  int num_pages = 10;

  /* Main inventory */
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot(&C.slots[i], get_slot_blueprint_idx(i));
  }
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot_hover(&C.slots[i], get_slot_blueprint_idx(i));
  }
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot_sel(&C.slots[i], get_slot_blueprint_idx(i));
  }
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot_hover_leg(&C.slots[i], get_slot_blueprint_idx(i));
  }

  /* Fixed slots */
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot(&C.fixed_slots[i], get_fixed_blueprint_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_hover(&C.fixed_slots[i], get_fixed_blueprint_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_sel(&C.fixed_slots[i], get_fixed_blueprint_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_hover_leg(&C.fixed_slots[i], get_fixed_blueprint_idx(i));
  }

  /* Page slots */
  for (int i = 0; i < NUM_PAGES; i++) {
    draw_slot(&C.page_slots[i], get_page_blueprint_idx(i));
  }
  if (C.editpage) {
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_hover(&C.page_slots[i], get_page_blueprint_idx(i));
    }
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_sel(&C.page_slots[i], get_page_blueprint_idx(i));
    }
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_hover_leg(&C.page_slots[i], get_page_blueprint_idx(i));
    }
  } else {
    for (int i = 0; i < NUM_PAGES; i++) {
      Rectangle r = C.page_slots[i].hitbox;
      // Color k = {130, 41, 8, 255};
      if (C.apage == i) {
        int x0 = r.x;
        int y0 = r.y + r.height;
        int w = r.width;
        DrawRectangle(r.x - 2, r.y - 2, 2, r.height + 2, k);
        DrawRectangle(r.x - 2, r.y - 2, r.width + 4, 2, k);
        DrawRectangle(r.x + r.width, r.y - 2, 2, r.height + 2, k);
      } else {
        int x0 = r.x;
        int y0 = r.y + r.height;
        int w = r.width;
        DrawRectangle(x0, y0, w, 2, k);
      }
    }
  }

  Texture sprites = ui_get_sprites();
  btn_draw_icon(&C.btn_editpage, 2, sprites, rect_editpage);

  btn_draw_text(&C.btn_close, 2, "CLOSE");
  btn_draw_text(&C.btn_delete, 2, "DELETE");
  btn_draw_text(&C.btn_rename, 2, "RENAME");
  btn_draw_text(&C.btn_use, 2, "USE");
  btn_draw_text(&C.btn_rot, 2, "ROTATE");
  //
  // draw_sel_item();

  if (ui_get_window() == WINDOW_BLUEPRINT) {
    btn_draw_legend(&C.btn_editpage, 2, "Edit page icons");
    btn_draw_legend(
        &C.btn_use, 2,
        "Pastes blueprint.\nYou can also right click to use a blueprint.");
    btn_draw_legend(&C.btn_rot, 2, "Rotates blueprint thumbnail (R)");
    btn_draw_legend(&C.btn_delete, 2, "Delete blueprint (DEL) (BACKSPACE)");
    btn_draw_legend(&C.btn_rename, 2, "Rename blueprint");
  }
}

int blueprint_create_and_open(int nl, Image* imgs, Image full) {
  int ibp = blueprint_create(nl, imgs, full);
  if (ibp == -1) {
    return -1;
  }
  ui_winpush(WINDOW_BLUEPRINT);
  update_layout();
  C.sel = ibp;
  gotopage_of(ibp);
  // text_modal_open(on_rename_accept, NULL, NULL);
  return ibp;
}
