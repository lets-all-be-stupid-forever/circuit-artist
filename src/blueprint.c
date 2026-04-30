#include "blueprint.h"

#include "assert.h"
#include "clipapi.h"
#include "font.h"
#include "fs.h"
#include "img.h"
#include "json.h"
#include "paths.h"
#include "stb_ds.h"
#include "stdio.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "wmain.h"

#define BLUEPRINT_FILE_1_1 "blueprints_1_1.json"
#define BLUEPRINT_FILE_v2 "blueprints_v2.json"

typedef struct {
  char* txt;
  Color c;
  int pad;
} LBItem;

typedef struct {
  // token = line + color, maybe image too
  LBItem* items;
} LegendBuilder;

void lb_init(LegendBuilder* lb) {
  *lb = (LegendBuilder){0};
  // TODO
}

void lb_add_line(LegendBuilder* lb, const char* txt, int pad, Color k) {
  LBItem item = {.txt = clone_string(txt), .c = k, .pad = pad};
  arrput(lb->items, item);
}

void lb_render(LegendBuilder* lb, Rectangle hitbox) {
  int w = -1;
  int lh = 20;
  int ni = arrlen(lb->items);
  int dbot = 10;
  int h = 0;
  for (int i = 0; i < ni; i++) {
    h += lh + 2 * lb->items[i].pad;
    const char* txt = lb->items[i].txt;
    int ww = get_rendered_text_size(txt).x;
    w = w > ww ? w : ww;
  }
  int th = h + dbot;
  w = 2 * w + 10;

  int y1 = hitbox.y;
  int xm = hitbox.x + hitbox.width / 2;
  int x0 = xm - w / 2;
  int y0 = y1 - th;

  Color bg = BLACK;
  bg.a = 225;
  rlPushMatrix();
  rlTranslatef(x0, y0 - 6, 0);
  DrawRectangle(0, 0, w, th, bg);
  rlScalef(2, 2, 1);
  for (int i = 0; i < ni; i++) {
    const char* txt = lb->items[i].txt;
    int ww = get_rendered_text_size(txt).x;
    int pad = lb->items[i].pad;
    int cx = (w / 2 - ww) / 2;
    int cy = 2 + pad / 2;
    font_draw_texture(txt, cx, cy, lb->items[i].c);
    rlTranslatef(0, lh / 2 + pad, 0);
  }
  rlPopMatrix();
}

void unload_lb(LegendBuilder* lb) {
  int n = arrlen(lb->items);
  for (int i = 0; i < n; i++) {
    free(lb->items[i].txt);
  }
  arrfree(lb->items);
}

void blueprint_draw_leg(Btn* b, Blueprint* s, int scale) {
  if (!b->hover) return;
  const char* hover_name = s->name;
  Rectangle r = b->hitbox;
  LegendBuilder lb = {0};
  char line[256];
  int k = 100;
  Color c2 = {k, k, k, 255};
  lb_init(&lb);
  lb_add_line(&lb, hover_name, 4, WHITE);
  // lb_add_line(&lb, TextFormat("w: %d h: %d", s->width, s->height), 2, c2);
  if (s->steam_author_name) {
    lb_add_line(&lb, TextFormat("by %s", s->steam_author_name), 2, c2);
  }
  // if (s->steam_id == 0) {
  //   const char* folder = &s->id[6];
  //   lb_add_line(&lb, TextFormat("folder %s", folder), 2, c2);
  // }
  lb_render(&lb, r);
  unload_lb(&lb);

#if 0
  rlPushMatrix();
  rlTranslatef(r.x, r.y - 24, 0);
  rlScalef(2, 2, 1);
  int w = get_rendered_text_size(hover_name).x;
  Color c = BLACK;
  c.a = 150;
  DrawRectangle(0, -2, w, 12 + 4, c);
  font_draw_texture_outlined(hover_name, 0, 0, WHITE, BLACK);
#endif
}

const char* get_blueprint_path(BlueprintStore* store) {
  return get_data_path(BLUEPRINT_FILE_v2);
}

static void add_bp_from_img_asset(BlueprintStore* store, const char* asset) {
  Image img = load_image_asset(asset);
  int nl = 0;
  Image tmp[MAX_LAYERS] = {0};
  image_decode_layers(img, &nl, tmp);
  blueprint_create(store, nl, tmp, img);
  for (int i = 0; i < nl; i++) {
    UnloadImage(tmp[i]);
  }
  UnloadImage(img);
}

void blueprint_store_swap(BlueprintStore* store, int i0, int i1) {
  Blueprint* tmp = store->blueprints[i0];
  store->blueprints[i0] = store->blueprints[i1];
  store->blueprints[i1] = tmp;
}

int get_page_blueprint_idx(BlueprintStore* store, int i) {
  int off = NUM_FIXED + NUM_PAGES * PAGESIZE;
  return off + i;
}

int get_page_slot_blueprint_idx(BlueprintStore* store, int page, int i) {
  int off = NUM_FIXED + page * PAGESIZE;
  return off + i;
}

static void initialize_bps(BlueprintStore* store) {
  add_bp_from_img_asset(store, "default_blueprints/default_page_1.png");
  add_bp_from_img_asset(store, "default_blueprints/default_page_2.png");
  add_bp_from_img_asset(store, "default_blueprints/default_bp1.png");
  add_bp_from_img_asset(store, "default_blueprints/default_bp2.png");
  add_bp_from_img_asset(store, "default_blueprints/default_bp3.png");
  add_bp_from_img_asset(store, "default_blueprints/default_bp4.png");
  blueprint_store_swap(store, 0, get_page_blueprint_idx(store, 0));
  blueprint_store_swap(store, 1, get_page_blueprint_idx(store, 1));
  blueprint_store_swap(store, 2, get_page_slot_blueprint_idx(store, 0, 0));
  blueprint_store_swap(store, 3, get_page_slot_blueprint_idx(store, 0, 1));
  blueprint_store_swap(store, 4, get_page_slot_blueprint_idx(store, 0, 2));
  blueprint_store_swap(store, 5, get_page_slot_blueprint_idx(store, 0, 3));
  blueprint_store_save(store);
}

int get_fixed_blueprint_idx(BlueprintStore* store, int i) { return i; }

int find_available_fixed_slot(BlueprintStore* store) {
  for (int i = 0; i < NUM_FIXED; i++) {
    int sidx = get_fixed_blueprint_idx(store, i);
    if (!store->blueprints[sidx]) return sidx;
  }
  return -1;
}

int find_available_page_slot(BlueprintStore* store, int page) {
  for (int i = 0; i < PAGESIZE; i++) {
    int sidx = get_page_slot_blueprint_idx(store, page, i);
    if (!store->blueprints[sidx]) return sidx;
  }
  return -1;
}

int find_first_available_slot(BlueprintStore* store, int apage) {
  int c = -1;
  c = find_available_fixed_slot(store);
  if (c >= 0) return c;
  c = find_available_page_slot(store, apage);
  if (c >= 0) return c;
  for (int i = 0; i < NUM_PAGES; i++) {
    c = find_available_page_slot(store, i);
    if (c >= 0) return c;
  }
  return -1;
}

Blueprint* find_or_create_bp(BlueprintStore* store, const char* id) {
  // Find an existing stub slot with the matching id.
  for (int i = 0; i < store->nbp; i++) {
    Blueprint* bp = store->blueprints[i];
    if (bp && str_match(bp->id, id)) {
      return bp;
    }
  }

  // No reserved slot — use any free slot.
  int slot = find_first_available_slot(store, 0);
  if (slot == -1) {
    return NULL;
  }
  Blueprint* bp = calloc(1, sizeof(Blueprint));
  bp->id = clone_string(id);
  store->blueprints[slot] = bp;
  return bp;
}

static void extract_steam_id_from_id(Blueprint* bp) {
  bp->steam_id = 0;
  if (starts_with(bp->id, "steam:")) {
    bp->steam_id = str2u64(&bp->id[6]);
  }
}

static void load_steam_author_if_applicable(Blueprint* bp) {
  if (bp->steam_id == 0) return;
  SteamMeta m = {0};
  if (load_steam_metadata(bp->folder, &m)) {
    bp->steam_author_id = m.author_id;
    bp->steam_author_name = clone_string(m.author_name);
    unload_steam_meta(&m);
  };
}

void inject_blueprint_from_folder(BlueprintStore* store, const char* id,
                                  const char* folder) {
  char meta_path[1024];
  char thumb_path[1024];
  snprintf(meta_path, sizeof(meta_path), "%s/meta.json", folder);
  snprintf(thumb_path, sizeof(thumb_path), "%s/thumb.png", folder);
  if (!FileExists(meta_path) || !FileExists(thumb_path)) return;
  json_object* meta = json_object_from_file(meta_path);
  if (!meta) {
    return;
  }
  Blueprint* bp = find_or_create_bp(store, id);
  if (bp) {
    json_read_str(meta, "lvl", &bp->lvl);
    json_read_str(meta, "name", &bp->name);
    json_read_str(meta, "desc", &bp->desc);
    json_read_int(meta, "width", &bp->width);
    json_read_int(meta, "height", &bp->height);
    json_read_int(meta, "layers", &bp->layers);
    bp->folder = clone_string(folder);
    bp->thumbnail = LoadTexture(thumb_path);
    extract_steam_id_from_id(bp);
    load_steam_author_if_applicable(bp);
  }
  json_object_put(meta);
}

// Scans blueprints_v2/ for subfolders and calls inject_blueprint_from_folder
// on each. Call this to pick up locally stored blueprints.
static void register_local_blueprints(BlueprintStore* store) {
  char v2dir[512];
  snprintf(v2dir, sizeof(v2dir), "%s", get_data_path("blueprints_v2"));
  if (!DirectoryExists(v2dir)) return;

  FilePathList dirs = LoadDirectoryFiles(v2dir);
  for (unsigned int i = 0; i < dirs.count; i++) {
    if (DirectoryExists(dirs.paths[i])) {
      char* base = os_path_basename(dirs.paths[i]);
      char* local_id = clone_string(TextFormat("local:%s", base));
      inject_blueprint_from_folder(store, local_id, dirs.paths[i]);
      free(local_id);
      free(base);
    }
  }
  UnloadDirectoryFiles(dirs);
}

static void blueprint_destroy(Blueprint* bp) {
  if (!bp) return;
  free(bp->id);
  free(bp->name);
  free(bp->desc);
  free(bp->lvl);
  free(bp->folder);
  free(bp->steam_author_name);
  UnloadTexture(bp->thumbnail);
  free(bp);
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

static void fix_bp_id_version(Blueprint* bp) {
  bool is_local = starts_with(bp->id, "local:");
  bool is_steam = starts_with(bp->id, "steam:");
  if (!is_local && !is_steam) {
    char* old_id = bp->id;
    bp->id = clone_string(TextFormat("local:%s", old_id));
    free(old_id);
  }
}

void blueprint_store_load(BlueprintStore* store) {
  if (!FileExists(get_blueprint_path(store))) {
    initialize_bps(store);
    return;
  }

  // Step 1: Parse blueprints_v2.json and reserve id-only stubs at the
  // recorded slot positions (preserves slot layout even before folders
  // are scanned).
  {
    json_object* root = json_object_from_file(get_blueprint_path(store));
    if (root) {
      json_object* bps = NULL;
      json_object_object_get_ex(root, "blueprints", &bps);
      if (bps) {
        int n = (int)json_object_array_length(bps);
        if (n > TOTAL_BLUEPRINTS) n = TOTAL_BLUEPRINTS;
        for (int i = 0; i < n; i++) {
          json_object* bp = json_object_array_get_idx(bps, i);
          if (!bp || json_object_get_type(bp) == json_type_null) continue;
          Blueprint* stub = calloc(1, sizeof(Blueprint));
          json_read_str(bp, "id", &stub->id);
          json_read_int(bp, "rot", &stub->rot);
          fix_bp_id_version(stub);
          store->blueprints[i] = stub;
        }
      }
      json_object_put(root);
    }
  }

  // Step 2: Scan actual folders and fill in (or add) blueprints.
  register_local_blueprints(store);
  steam_load_blueprints();

  // Step 3: Remove stubs that were never matched to a real folder
  // (thumbnail.id == 0 means no texture was loaded).
  for (int i = 0; i < TOTAL_BLUEPRINTS; i++) {
    Blueprint* bp = store->blueprints[i];
    if (bp && bp->thumbnail.id == 0) {
      blueprint_destroy(bp);
      store->blueprints[i] = NULL;
    }
  }
}

char* gen_bp_id(Blueprint* bp) {
  char out[100];
  if (bp->steam_id) {
    snprintf(out, sizeof(out), "steam:%llu", bp->steam_id);
    return clone_string(out);
  }
  char* f = os_path_basename(bp->folder);
  snprintf(out, sizeof(out), "local:%s", f);
  free(f);
  return clone_string(out);
}

// blueprints_v2.json: positional array, each slot is null or {id, rot}
void blueprint_store_save(BlueprintStore* store) {
  json_object* root = json_object_new_object();
  if (!root) {
    ui_crash("Couldn't create blueprint inventory file.");
    return;
  }
  json_object_object_add(root, "version", json_object_new_int(2));
  json_object* bps = json_object_new_array();
  for (int i = 0; i < store->nbp; i++) {
    Blueprint* s = store->blueprints[i];
    json_object* bp;
    if (!s) {
      bp = json_object_new_null();
    } else {
      bp = json_object_new_object();
      char* id = gen_bp_id(s);
      json_write_str(bp, "id", id);
      free(id);
      json_write_int(bp, "rot", s->rot);
    }
    json_object_array_add(bps, bp);
  }
  json_object_object_add(root, "blueprints", bps);
  if (json_object_to_file_ext(get_blueprint_path(store), root,
                              JSON_C_TO_STRING_PRETTY) < 0) {
    fprintf(stderr, "Error writing blueprint inventory\n");
    ui_crash("Error writing blueprint inventory\n");
  }
  json_object_put(root);
}

void blueprint_save_meta(Blueprint* s) {
  assert(s->steam_id == 0);
  assert(s->folder);
  json_object* meta = json_object_new_object();
  json_write_str(meta, "name", s->name);
  json_write_str(meta, "desc", s->desc);
  json_write_int(meta, "width", s->width);
  json_write_int(meta, "height", s->height);
  json_write_int(meta, "layers", s->layers);
  if (s->lvl) {
    json_write_str(meta, "lvl", s->lvl);
  }
  json_object_to_file_ext(TextFormat("%s/meta.json", s->folder), meta,
                          JSON_C_TO_STRING_PRETTY);
  json_object_put(meta);
}

const char* blueprint_fname_full(Blueprint* s) {
  return TextFormat("%s/full.png", s->folder);
}

const char* blueprint_fname_thumbnail(Blueprint* s) {
  return TextFormat("%s/thumb.png", s->folder);
}

int blueprint_create(BlueprintStore* store, int nl, Image* imgs, Image full) {
  int ibp = find_first_available_slot(store, 0);
  if (ibp == -1) return -1;
  assert(!store->blueprints[ibp]);

  Image thumb = gen_thumbnail(nl, imgs, 64, 64, false);
  Blueprint* bp = calloc(1, sizeof(Blueprint));
  bp->name = clone_string("My Blueprint");
  bp->steam_id = 0;
  bp->desc = NULL;
  bp->layers = nl;
  bp->width = imgs[0].width;
  bp->height = imgs[0].height;
  char* id = clone_string(randid());
  bp->folder = abs_path(get_data_path(TextFormat("blueprints_v2/%s", id)));
  free(id);
  store->blueprints[ibp] = bp;

  // Create per-blueprint folder and write files
  MakeDirectory(bp->folder);
  ExportImage(full, blueprint_fname_full(bp));
  ExportImage(thumb, blueprint_fname_thumbnail(bp));
  blueprint_save_meta(bp);

  bp->thumbnail = LoadTextureFromImage(thumb);
  UnloadImage(thumb);

  blueprint_store_save(store);
  return ibp;
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
static void migrate_v1_to_v2(BlueprintStore* store) {
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

static void ensure_blueprints_folder_exists(BlueprintStore* store) {
  if (!DirectoryExists(get_data_path("blueprints_v2"))) {
    MakeDirectory(get_data_path("blueprints_v2"));
  }
}

void blueprint_store_init(BlueprintStore* store) {
  int nbp = TOTAL_BLUEPRINTS;
  store->blueprints = calloc(nbp, sizeof(Blueprint*));
  store->nbp = nbp;

  migrate_v1_to_v2(store);
  ensure_blueprints_folder_exists(store);
  blueprint_store_load(store);
}

void blueprint_store_rm(BlueprintStore* store, int idx) {
  Blueprint* s = store->blueprints[idx];
  if (s->steam_id) {
#ifdef WITH_STEAM
    steam_unsubscribe_item(s->steam_id);
#endif
  } else {
    blueprint_rm_files(s);
  }
  blueprint_destroy(s);
  store->blueprints[idx] = NULL;
  blueprint_store_save(store);
}

void blueprint_rename(Blueprint* bp, const char* new_name) {
  free(bp->name);
  bp->name = clone_string(new_name);
  blueprint_save_meta(bp);
}

Blueprint* get_blueprint(BlueprintStore* store, int idx) {
  return store->blueprints[idx];
}

void blueprint_paste(Blueprint* bp) {
  main_paste_file(blueprint_fname_full(bp), bp->rot);
}

void blueprint_rot(Blueprint* bp) { bp->rot = (bp->rot + 1) % 4; }

void blueprint_set_desc(Blueprint* bp, const char* new_desc) {
  free(bp->desc);
  bp->desc = clone_string(new_desc);
  blueprint_save_meta(bp);
}

void blueprint_draw(Btn* b, Blueprint* s, int scale) {
  Color c = BLACK;
  c.a = 128;
  Rectangle slot = b->hitbox;
  Texture2D sprites = ui_get_sprites();
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
    int xx = x + (tw - scale * sw) / 2;
    int yy = y + (th - scale * sh) / 2;
    float hh = th / 2.0;
    float ww = tw / 2.0;
    rlPushMatrix();
    rlTranslatef(xx, yy, 0);
    // DrawTexture(s->thumbnail, 0, 0, WHITE);
    Rectangle src = {0, 0, sw, sh};
    Rectangle dst = {0, 0, scale * sw, scale * sh};
    rlTranslatef(scale * sw / 2, scale * sh / 2, 0);
    DrawTexturePro(s->thumbnail, src, dst,
                   (Vector2){scale * sw / 2, scale * sh / 2}, s->rot * 90,
                   WHITE);
    rlPopMatrix();
    if (s->steam_id) {
      rlPushMatrix();
      rlTranslatef(x, y, 0);
      Color clr = {0, 255, 0, 30};
      DrawRectangle(0, 0, tw, th, clr);
      Rectangle sprite_dst = {0, 0, 14, 14};
      DrawTexturePro(sprites, rect_steam_big, sprite_dst, (Vector2){2, 2}, 0,
                     WHITE);
      rlPopMatrix();
    }
  }
}

void blueprint_update_thumbnail(Blueprint* bp, int nl, Image* imgs) {
  Image thumb = gen_thumbnail(nl, imgs, 64, 64, false);
  ExportImage(thumb, blueprint_fname_thumbnail(bp));
  if (bp->thumbnail.width > 0) UnloadTexture(bp->thumbnail);
  bp->thumbnail = LoadTextureFromImage(thumb);
  UnloadImage(thumb);
}

bool blueprint_can_delete(Blueprint* s) {
  return main_get_editting_blueprint() != s;
}

void blueprint_copy_to_clipboard(Blueprint* s) {
  Image img = LoadImage(blueprint_fname_full(s));
  image_to_clipboard(img);
  UnloadImage(img);
}
