#include "win_blueprint.h"

#include "assert.h"
#include "blueprint.h"
#include "font.h"
#include "fs.h"
#include "img.h"
#include "json.h"
#include "layout.h"
#include "msg.h"
#include "paths.h"
#include "sound.h"
#include "stdio.h"
#include "stdlib.h"
#include "steam.h"
#include "string.h"
#include "ui.h"
#include "utils.h"
#include "wdialog.h"
#include "widgets.h"
#include "win_bpdetail.h"
#include "win_msg.h"
#include "win_mtext.h"
#include "win_pubform.h"
#include "wmain.h"
#include "wtext.h"

/* Blueprints are mapped as: (i) () Staging, (MxNinventory) (N pages) */
static struct {
  Layout* layout;
  Label pages_cap;
  Label staging_cap;
  Label slots_cap;
  Rectangle modal;
  BlueprintStore store;
  // Blueprint* blueprints[TOTAL_BLUEPRINTS];
  Btn slots[PAGESIZE];
  Btn page_slots[NUM_PAGES];
  Btn fixed_slots[NUM_FIXED];
  Btn btn_close;
  Btn btn_editpage;
  Btn btn_steam;
  int sel;
  int apage; /* active page */
  bool editpage;
} C = {0};

static int get_slot_blueprint_idx(int i) {
  return get_page_slot_blueprint_idx(&C.store, C.apage, i);
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
  Layout* l = C.layout;
  layout_update_offset(l);
  C.modal = layout_rect(l, "window");
  C.btn_close.hitbox = layout_rect(l, "btn_close");
  C.pages_cap.hitbox = layout_rect(l, "pages_cap");
  C.btn_editpage.hitbox = layout_rect(l, "btn_pages");
  C.btn_steam.hitbox = layout_rect(l, "btn_steam");
  C.slots_cap.hitbox = layout_rect(l, "slots_cap");
  C.staging_cap.hitbox = layout_rect(l, "staging_cap");
  Rectangle slots = layout_rect(l, "slots");
  Rectangle staging = layout_rect(l, "staging");
  Rectangle pages = layout_rect(l, "pages");
  int rows = 7;
  int cols = 15;
  fix_slot_layout(C.slots, slots.x, slots.y, rows, cols);
  fix_slot_layout(C.fixed_slots, staging.x, staging.y, rows, 2);
  fix_slot_layout(C.page_slots, pages.x, pages.y, 1, cols);
}

static void update_labels() {
  label_set_text(&C.pages_cap, C.editpage ? "Pages (editing)" : "Pages");
  label_set_text(&C.slots_cap, "Inventory");
  label_set_text(&C.staging_cap, "Fixed Slots");
}

void win_blueprint_init() {
  C.layout = easy_load_layout("blueprint");
  blueprint_store_init(&C.store);
  update_labels();
}

void win_blueprint_open() {
  ui_winpush(WINDOW_BLUEPRINT);
  update_layout();
  C.sel = -1;
}

static void set_page(int page) { C.apage = page; }

static void gotopage_of(int ibp) {
  if (ibp < NUM_FIXED) return;
  int p = (ibp - NUM_FIXED) / PAGESIZE;
  set_page(p);
}

static void on_rename_accept(void* ctx, const char* txt) {
  if (C.sel != -1 && get_blueprint(&C.store, C.sel)) {
    blueprint_rename(get_blueprint(&C.store, C.sel), txt);
  }
}

static void use_blueprint(int ibp) {
  blueprint_paste(get_blueprint(&C.store, ibp));
  play_sound_click();
  ui_winpop();
  blueprint_store_save(&C.store);
  return;
}

static void update_slot(Btn* b, Vector2 mouse, int sidx) {
  b->hover = rect_hover(b->hitbox, mouse);
  bool isfixed = sidx < NUM_FIXED;
  BlueprintStore* store = &C.store;
  if (b->hover) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      if (get_blueprint(store, sidx)) {
        use_blueprint(sidx);
      }
    }
    if (C.sel != -1 && sidx != C.sel) {
      ui_set_cursor(MOUSE_SWITCH);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      bool empty = get_blueprint(store, sidx) == NULL;
      if (C.sel != -1) {
        if (sidx == C.sel) {
          win_bpdetail_open(get_blueprint(store, sidx), store, sidx);
          C.sel = -1;
          play_sound_click();
          return;
        } else {
          blueprint_store_swap(store, C.sel, sidx);
          C.sel = -1;
          play_sound_click();
        }
      } else {
        if (get_blueprint(store, sidx)) {
          play_sound_click();
          if (is_control_down()) {
            /* Tries to move to fixed slot*/
            if (!isfixed) {
              int tgt = find_available_fixed_slot(store);
              if (tgt >= 0) {
                blueprint_store_swap(store, sidx, tgt);
              }
            } else {
              int tgt = find_available_page_slot(store, C.apage);
              if (tgt >= 0) {
                blueprint_store_swap(store, sidx, tgt);
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
static void on_confirm_delete(int r) {
  if (r != 0) return;
  blueprint_store_rm(&C.store, C.sel);
  blueprint_store_save(&C.store);
  msg_add("Blueprint deleted.", 4);
  C.sel = -1;
}

void win_blueprint_update() {
  update_layout();
  bool key_escape = IsKeyPressed(KEY_ESCAPE);
  bool key_q = IsKeyPressed(KEY_Q);
  if (btn_update(&C.btn_close) || (key_escape && C.sel == -1) || key_q) {
    ui_winpop();
    blueprint_store_save(&C.store);
    return;
  }

  /* Escape de-selects blueprint */
  if (key_escape && C.sel != -1) {
    C.sel = -1;
  }

  if (C.sel != -1) {
    if (IsKeyPressed(KEY_R)) {
      blueprint_rot(get_blueprint(&C.store, C.sel));
      return;
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
      Blueprint* s = get_blueprint(&C.store, C.sel);
      if (!blueprint_can_delete(s)) {
        win_msg_open_text("Blueprint can't be deleted while being editted.",
                          NULL);
        return;
      }
      dialog_open(TextFormat("Delete blueprint %s?", s->name), "Delete",
                  "Cancel", NULL, on_confirm_delete);
      return;
    }
    if (IsKeyPressed(KEY_ENTER)) {
      use_blueprint(C.sel);
      return;
    }
    if (IsKeyPressed(KEY_F2)) {
      text_modal_open(on_rename_accept, NULL,
                      get_blueprint(&C.store, C.sel)->name);
      return;
    }
  }
  C.btn_steam.disabled = !is_steam_on();
  if (btn_update(&C.btn_steam)) {
    steam_open_overlay_blueprints();
    return;
  }

  if (btn_update(&C.btn_editpage)) {
    C.editpage = !C.editpage;
    if (C.sel >= NUM_FIXED + NUM_PAGES * PAGESIZE) {
      C.sel = -1;
    }
    return;
  }
  C.btn_editpage.toggled = C.editpage;

  Vector2 mouse = GetMousePosition();
  for (int i = 0; i < PAGESIZE; i++) {
    update_slot(&C.slots[i], mouse, get_slot_blueprint_idx(i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    update_slot(&C.fixed_slots[i], mouse, get_fixed_blueprint_idx(&C.store, i));
  }
  for (int i = 0; i < NUM_PAGES; i++) {
    update_page_slot(&C.page_slots[i], mouse,
                     get_page_blueprint_idx(&C.store, i));
  }
  update_labels();
}

static void draw_sel_item() {
  if (C.sel == -1) return;
  Vector2 mouse = GetMousePosition();

  rlPushMatrix();
  rlTranslatef(mouse.x, mouse.y + 15, 0);
  Blueprint* s = get_blueprint(&C.store, C.sel);
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

void add_steam_blueprint(const char* folder, u64 id) {
  char bpid[256];
  snprintf(bpid, sizeof(bpid), "steam:%" PRIu64, id);
  inject_blueprint_from_folder(&C.store, bpid, folder);
}

void draw_slot(Btn* b, int sidx) {
  blueprint_draw(b, get_blueprint(&C.store, sidx), 1);
}

static void draw_slot_hover(Btn* b, int sidx) {
  bool hover = b->hover;
  Blueprint* s = get_blueprint(&C.store, sidx);
  if (hover) {
    Rectangle r = b->hitbox;
    Rectangle r2 = {r.x - 2, r.y - 2, r.width + 4, r.height + 4};
    Color c = WHITE;
    c.a = 100;
    DrawRectangleLinesEx(r2, 2, c);
  }
}

static void draw_slot_hover_leg(Btn* b, int sidx) {
  bool hover = b->hover;
  Blueprint* s = C.store.blueprints[sidx];
  if (!s) return;
  blueprint_draw_leg(b, s, 1);
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

void win_blueprint_draw() {
  draw_win(C.modal, "BLUEPRINTS");

  Color k = GetColor(0xBB830BFF);
  label_draw(&C.slots_cap);
  label_draw(&C.pages_cap);
  label_draw(&C.staging_cap);

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

  BlueprintStore* store = &C.store;
  /* Fixed slots */
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot(&C.fixed_slots[i], get_fixed_blueprint_idx(store, i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_hover(&C.fixed_slots[i], get_fixed_blueprint_idx(store, i));
  }
  for (int i = 0; i < NUM_FIXED; i++) {
    draw_slot_sel(&C.fixed_slots[i], get_fixed_blueprint_idx(store, i));
  }

  /* Page slots */
  for (int i = 0; i < NUM_PAGES; i++) {
    draw_slot(&C.page_slots[i], get_page_blueprint_idx(store, i));
  }
  if (C.editpage) {
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_hover(&C.page_slots[i], get_page_blueprint_idx(store, i));
    }
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_sel(&C.page_slots[i], get_page_blueprint_idx(store, i));
    }
    for (int i = 0; i < NUM_PAGES; i++) {
      draw_slot_hover_leg(&C.page_slots[i], get_page_blueprint_idx(store, i));
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
  btn_draw_text(&C.btn_steam, 2, "BROWSE WORKSHOP");

  if (ui_get_window() == WINDOW_BLUEPRINT) {
    btn_draw_legend(&C.btn_editpage, 2, "Edit page icons");
    for (int i = 0; i < NUM_FIXED; i++) {
      draw_slot_hover_leg(&C.fixed_slots[i], get_fixed_blueprint_idx(store, i));
    }
    for (int i = 0; i < PAGESIZE; i++) {
      draw_slot_hover_leg(&C.slots[i], get_slot_blueprint_idx(i));
    }
  }
}

int blueprint_create_and_open(int nl, Image* imgs, Image full) {
  int ibp = blueprint_create(&C.store, nl, imgs, full);
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
