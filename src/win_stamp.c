#include "win_stamp.h"

#include "assert.h"
#include "font.h"
#include "img.h"
#include "json.h"
#include "layout.h"
#include "msg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ui.h"
#include "utils.h"
#include "wdialog.h"
#include "widgets.h"
#include "wmain.h"
#include "wtext.h"

#define S_COLS 15
#define S_ROWS 7
#define F_COLS 2
#define NUM_PAGES S_COLS
#define NUM_FIXED (F_COLS * S_ROWS)
#define PAGESIZE (S_COLS * S_ROWS)
#define TOTAL_STAMPS (NUM_FIXED + NUM_PAGES * PAGESIZE + NUM_PAGES)

/* stamps are mapped as: (i) () Staging, (MxNinventory) (N pages) */
static struct {
  Stamp* stamps[TOTAL_STAMPS];
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

#define LAYOUT(name) roff(off, WIN_STAMP_##name)

static void stamp_save() {
  json_object *stamps, *stamp, *name, *id, *rot;
  json_object* root = json_object_new_object();
  if (!root) {
    ui_crash("Couldn't create stamp inventory file.");
    return;
  }
  json_object_object_add(root, "version", json_object_new_int(0));
  stamps = json_object_new_array();
  int total_stamps = TOTAL_STAMPS;
  for (int i = 0; i < total_stamps; i++) {
    Stamp* s = C.stamps[i];
    if (!s) {
      stamp = json_object_new_null();
    } else {
      stamp = json_object_new_object();
      id = json_object_new_string(s->id);
      name = json_object_new_string(s->name);
      rot = json_object_new_int(s->rot);
      json_object_object_add(stamp, "id", id);
      json_object_object_add(stamp, "name", name);
      json_object_object_add(stamp, "rot", rot);
      printf("STAMP %d %s\n", i, s->id);
    }
    json_object_array_add(stamps, stamp);
  }
  json_object_object_add(root, "stamps", stamps);

  if (json_object_to_file_ext(STAMPS_FILE, root, JSON_C_TO_STRING_PRETTY) < 0) {
    fprintf(stderr, "Error writing to file\n");
    json_object_put(root);
    ui_crash("Error creating save file\n");
    return;
  }
  json_object_put(root);
}

const char* stamp_fname(Stamp* s) {
  return TextFormat("../blueprints/%s_full.png", s->id);
}

const char* stamp_fname_thumbnail(Stamp* s) {
  return TextFormat("../blueprints/%s_thumb.png", s->id);
}

static int get_page_slot_stamp_idx(int page, int i) {
  int off = NUM_FIXED + page * PAGESIZE;
  return off + i;
}

static int get_slot_stamp_idx(int i) {
  return get_page_slot_stamp_idx(C.apage, i);
}

static int get_page_stamp_idx(int i) {
  int off = NUM_FIXED + NUM_PAGES * PAGESIZE;
  return off + i;
}

static int get_fixed_stamp_idx(int i) { return i; }

static int find_available_fixed_slot() {
  for (int i = 0; i < NUM_FIXED; i++) {
    int sidx = get_fixed_stamp_idx(i);
    if (!C.stamps[sidx]) return sidx;
  }
  return -1;
}

static int find_available_page_slot(int page) {
  for (int i = 0; i < PAGESIZE; i++) {
    int sidx = get_page_slot_stamp_idx(page, i);
    if (!C.stamps[sidx]) return sidx;
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

static int stamp_create(int nl, Image* imgs, Image full) {
  int istamp = find_first_available_slot();
  if (istamp == -1) {
    return -1;
  }
  assert(!C.stamps[istamp]);
  Image thumb = gen_thumbnail(nl, imgs, 64, 64);
  char* id = clone_string(randid());
  // int istamp = get_next_available_loc();
  assert(istamp >= 0);
  Stamp* stamp = calloc(1, sizeof(Stamp));
  C.stamps[istamp] = stamp;
  stamp->id = id;
  ExportImage(thumb, stamp_fname_thumbnail(stamp));
  ExportImage(full, stamp_fname(stamp));

  stamp->thumbnail = LoadTextureFromImage(thumb);

  // stamp->name = clone_string("My Blueprint");
  stamp->name = clone_string("");
  UnloadImage(thumb);
  stamp_save();
  return istamp;
}

static void swap_stamps(int i0, int i1) {
  Stamp* tmp = C.stamps[i0];
  C.stamps[i0] = C.stamps[i1];
  C.stamps[i1] = tmp;
}

static void add_bp_from_img(const char* path) {
  Image img = LoadImage(path);
  int nl = 0;
  Image tmp[MAX_LAYERS] = {0};
  image_decode_layers(img, &nl, tmp);
  stamp_create(nl, tmp, img);
  for (int i = 0; i < nl; i++) {
    UnloadImage(tmp[i]);
  }
  UnloadImage(img);
}

static void initialize_stamps() {
  add_bp_from_img("../assets/default_page_1.png");
  add_bp_from_img("../assets/default_page_2.png");
  add_bp_from_img("../assets/default_bp1.png");
  add_bp_from_img("../assets/default_bp2.png");
  add_bp_from_img("../assets/default_bp3.png");
  add_bp_from_img("../assets/default_bp4.png");
  swap_stamps(0, get_page_stamp_idx(0));
  swap_stamps(1, get_page_stamp_idx(1));
  swap_stamps(2, get_page_slot_stamp_idx(0, 0));
  swap_stamps(3, get_page_slot_stamp_idx(0, 1));
  swap_stamps(4, get_page_slot_stamp_idx(0, 2));
  swap_stamps(5, get_page_slot_stamp_idx(0, 3));
  stamp_save();
}

void stamp_load() {
  if (!FileExists(STAMPS_FILE)) {
    initialize_stamps();
    return;
  }
  json_object* stamps;
  json_object *stamp, *id, *name, *rot;
  json_object* root = json_object_from_file(STAMPS_FILE);
  assert(root);
  if (json_object_object_get_ex(root, "stamps", &stamps)) {
    int nl = json_object_array_length(stamps);
    for (int i = 0; i < nl; i++) {
      stamp = json_object_array_get_idx(stamps, i);
      if (stamp) {
        Stamp* s = calloc(1, sizeof(Stamp));
        bool ok = true;
        ok = ok && json_object_object_get_ex(stamp, "id", &id);
        ok = ok && json_object_object_get_ex(stamp, "name", &name);
        if (json_object_object_get_ex(stamp, "rot", &rot)) {
          s->rot = json_object_get_int(rot);
        };
        s->id = clone_string(json_object_get_string(id));
        s->name = clone_string(json_object_get_string(name));
        // Image img = LoadImage(stamp_fname_thumbnail(s));
        // image_remove_blacks(&img);
        // s->thumbnail = LoadTextureFromImage(img);
        // UnloadImage(img);
        s->thumbnail = LoadTexture(stamp_fname_thumbnail(s));
        C.stamps[i] = s;
      }
    }
  }
  json_object_put(root);
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
  C.modal = WIN_STAMP_win_stamp;
  Vector2 off = find_modal_off(C.modal);
  C.modal = LAYOUT(win_stamp);
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

void win_stamp_init() { stamp_load(); }

void win_stamp_open() {
  ui_winpush(WINDOW_STAMP);
  update_layout();
  C.sel = -1;
}

static void on_confirm_delete(int r) {
  if (r == 1 || r == -1) return;
  int i = C.sel;
  if (i != -1 && C.stamps[i]) {
    Stamp* s = C.stamps[i];
    delete_file(stamp_fname_thumbnail(s));
    delete_file(stamp_fname(s));
    UnloadTexture(s->thumbnail);
    free(s->id);
    free(s->name);
    free(C.stamps[i]);
    C.stamps[i] = NULL;
    msg_add("Stamp deleted.", 4);
    C.sel = -1;
  }
}

static void set_page(int page) { C.apage = page; }

static void gotopage_of(int istamp) {
  if (istamp < NUM_FIXED) return;
  int p = (istamp - NUM_FIXED) / PAGESIZE;
  set_page(p);
}

static void on_rename_accept(void* ctx, const char* txt) {
  int i = C.sel;
  if (i != -1 && C.stamps[i]) {
    Stamp* s = C.stamps[i];
    free(s->name);
    s->name = clone_string(txt);
    // msg_add("Stamp renamed.", 4);
    C.sel = -1;
  }
}

static void use_stamp(int istamp) {
  Stamp* s = C.stamps[istamp];
  assert(s);
  main_paste_file(stamp_fname(s), s->rot);
  on_click();
  ui_winpop();
  stamp_save();
  return;
}

static void update_slot(Btn* b, Vector2 mouse, int sidx) {
  b->hover = rect_hover(b->hitbox, mouse);
  bool isfixed = sidx < NUM_FIXED;
  if (b->hover) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      if (C.stamps[sidx]) {
        use_stamp(sidx);
      }
    }
    if (C.sel != -1 && sidx != C.sel) {
      ui_set_cursor(MOUSE_SWITCH);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      bool empty = C.stamps[sidx] == NULL;
      if (C.sel != -1) {
        if (sidx == C.sel) {
          // use_sel();
          C.sel = -1;
          on_click();
          return;
        } else {
          swap_stamps(C.sel, sidx);
          C.sel = -1;
          on_click();
        }
      } else {
        if (C.stamps[sidx]) {
          on_click();
          if (is_control_down()) {
            /* Tries to move to fixed slot*/
            if (!isfixed) {
              int tgt = find_available_fixed_slot();
              if (tgt >= 0) {
                swap_stamps(sidx, tgt);
              }
            } else {
              int tgt = find_available_page_slot(C.apage);
              if (tgt >= 0) {
                swap_stamps(sidx, tgt);
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
        on_click();
        int ipage = sidx - NUM_FIXED - NUM_PAGES * PAGESIZE;
        set_page(ipage);
      }
    }
  }
}
static void on_del_stamp() {
  Stamp* s = C.stamps[C.sel];
  dialog_open(TextFormat("Delete blueprint %s ?", s->name), "Delete", "Cancel",
              NULL, on_confirm_delete);
  assert(s);
}

static void rot_stamp() {
  C.stamps[C.sel]->rot = (C.stamps[C.sel]->rot + 1) % 4;
}

void win_stamp_update() {
  update_layout();
  bool key_escape = IsKeyPressed(KEY_ESCAPE);
  bool key_del = IsKeyPressed(KEY_DELETE);
  bool key_backspace = IsKeyPressed(KEY_BACKSPACE);
  bool key_q = IsKeyPressed(KEY_Q);
  bool key_r = IsKeyPressed(KEY_R);
  if (btn_update(&C.btn_close) || (key_escape && C.sel == -1) || key_q) {
    ui_winpop();
    stamp_save();
    return;
  }

  if (key_r && C.sel != -1) {
    rot_stamp();
    return;
  }

  if (btn_update(&C.btn_rot)) {
    rot_stamp();
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
    text_modal_open(on_rename_accept, NULL, C.stamps[C.sel]->name);
    return;
  }

  if (btn_update(&C.btn_use)) {
    use_stamp(C.sel);
    return;
  }

  if (btn_update(&C.btn_delete)) {
    on_del_stamp();
    return;
  }
  if (C.sel != -1 && (key_del || key_backspace)) {
    on_del_stamp();
    return;
  }

  Vector2 mouse = GetMousePosition();
  for (int i = 0; i < PAGESIZE; i++) {
    update_slot(&C.slots[i], mouse, get_slot_stamp_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    update_slot(&C.fixed_slots[i], mouse, get_fixed_stamp_idx(i));
  }
  for (int i = 0; i < NUM_PAGES; i++) {
    update_page_slot(&C.page_slots[i], mouse, get_page_stamp_idx(i));
  }
}

static void draw_sel_item() {
  if (C.sel == -1) return;
  Vector2 mouse = GetMousePosition();

  rlPushMatrix();
  rlTranslatef(mouse.x, mouse.y + 15, 0);
  Stamp* s = C.stamps[C.sel];
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
  Stamp* s = C.stamps[sidx];
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
  Stamp* s = C.stamps[sidx];
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
  Stamp* s = C.stamps[sidx];
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

void win_stamp_draw() {
  draw_win(C.modal, "BLUEPRINTS");

  Color k = GetColor(0xBB830BFF);
  draw_labels();

  int num_pages = 10;

  /* Main inventory */
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot(&C.slots[i], get_slot_stamp_idx(i));
  }
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot_hover(&C.slots[i], get_slot_stamp_idx(i));
  }
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot_sel(&C.slots[i], get_slot_stamp_idx(i));
  }
  for (int i = 0; i < PAGESIZE; i++) {
    draw_slot_hover_leg(&C.slots[i], get_slot_stamp_idx(i));
  }

  /* Fixed slots */
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot(&C.fixed_slots[i], get_fixed_stamp_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_hover(&C.fixed_slots[i], get_fixed_stamp_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_sel(&C.fixed_slots[i], get_fixed_stamp_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_hover_leg(&C.fixed_slots[i], get_fixed_stamp_idx(i));
  }

  /* Page slots */
  for (int i = 0; i < NUM_PAGES; i++) {
    draw_slot(&C.page_slots[i], get_page_stamp_idx(i));
  }
  if (C.editpage) {
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_hover(&C.page_slots[i], get_page_stamp_idx(i));
    }
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_sel(&C.page_slots[i], get_page_stamp_idx(i));
    }
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_hover_leg(&C.page_slots[i], get_page_stamp_idx(i));
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

  if (ui_get_window() == WINDOW_STAMP) {
    btn_draw_legend(&C.btn_editpage, 2, "Edit page icons");
    btn_draw_legend(
        &C.btn_use, 2,
        "Pastes blueprint.\nYou can also right click to use a blueprint.");
    btn_draw_legend(&C.btn_rot, 2, "Rotates blueprint thumbnail (R)");
    btn_draw_legend(&C.btn_delete, 2, "Delete blueprint (DEL) (BACKSPACE)");
    btn_draw_legend(&C.btn_rename, 2, "Rename blueprint");
  }
}

int stamp_create_and_open(int nl, Image* imgs, Image full) {
  int istamp = stamp_create(nl, imgs, full);
  if (istamp == -1) {
    return -1;
  }
  ui_winpush(WINDOW_STAMP);
  update_layout();
  C.sel = istamp;
  gotopage_of(istamp);
  // text_modal_open(on_rename_accept, NULL, NULL);
  return istamp;
}
