#include "win_bpdetail.h"

#include "assert.h"
#include "blueprint.h"
#include "common.h"
#include "fs.h"
#include "img.h"
#include "layout.h"
#include "msg.h"
#include "paths.h"
#include "raylib.h"
#include "rlgl.h"
#include "sound.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "wdialog.h"
#include "widgets.h"
#include "win_msg.h"
#include "win_pubform.h"
#include "wmain.h"
#include "wtext.h"

static struct {
  Layout* layout;
  Rectangle modal;
  Rectangle rect_thumbnail;
  Btn btn_edit_title;
  Btn btn_rotate;
  Btn btn_use;
  Btn btn_open_steam;
  Btn btn_close;
  Btn btn_load;
  Btn btn_delete;
  Btn btn_publish;
  Btn btn_thumb;
  Btn btn_copy;
  Label lab_author;
  Label lab_props;
  Label lab_name;
  Blueprint* bp;
  BlueprintStore* store;
  bool delete_on_close;
  int idx;
  // Rectangle sep;
} C = {0};

static void update_layout() {
  layout_update_offset(C.layout);
  C.modal = layout_rect(C.layout, "window");
  C.btn_close.hitbox = layout_rect(C.layout, "btn_close");
  C.btn_use.hitbox = layout_rect(C.layout, "btn_use");
  C.btn_open_steam.hitbox = layout_rect(C.layout, "btn_open_steam");
  C.btn_delete.hitbox = layout_rect(C.layout, "btn_delete");
  C.btn_publish.hitbox = layout_rect(C.layout, "btn_publish");
  C.btn_rotate.hitbox = layout_rect(C.layout, "btn_rotate");
  C.btn_load.hitbox = layout_rect(C.layout, "btn_load");
  C.btn_edit_title.hitbox = layout_rect(C.layout, "btn_edit_title");
  C.btn_copy.hitbox = layout_rect(C.layout, "btn_copy");
  C.btn_thumb.hitbox = layout_rect(C.layout, "rect_thumbnail");
  // C.sep = layout_rect(C.layout, "sep");
  C.lab_author.hitbox = layout_rect(C.layout, "author");
  C.lab_name.hitbox = layout_rect(C.layout, "name");
  C.lab_props.hitbox = layout_rect(C.layout, "props");
}

void win_bpdetail_open(Blueprint* bp, BlueprintStore* store, int idx) {
  if (!C.layout) {
    C.layout = easy_load_layout("bpdetail");
  }
  C.bp = bp;
  C.store = store;
  C.idx = idx;
  update_layout();
  ui_winpush(WINDOW_BPDETAIL);
  // label_set_text(&C.lab_name, bp->name);
  label_set_text(&C.lab_author,
                 bp->steam_author_name ? bp->steam_author_name : "");
  label_set_text(&C.lab_props,
                 TextFormat("w: %d h: %d", bp->width, bp->height));
}

static void on_rename_accept(void* ctx, const char* txt) {
  blueprint_rename(C.bp, txt);
}

static void on_confirm_delete(int r) {
  if (r != 0) return;
  C.delete_on_close = true;
  ui_winpop();
}

static void do_publish() {
  Blueprint* bp = C.bp;
  Image full = LoadImage(blueprint_fname_full(bp));
  char* thumb_path = create_temp_thumbnail(full);
  char* thumb_path_abs = abs_path(thumb_path);
  free(thumb_path);
  UnloadImage(full);
  PubformParams p = {0};
  char* folder = abs_path(bp->folder);
  p.folder = folder;
  p.default_title = bp->name ? bp->name : "My Blueprint";
  p.default_desc = bp->desc ? bp->desc : "";
  p.default_thumbnail_path = thumb_path_abs;
  win_pubform_open(p);
  free(thumb_path_abs);
  free(folder);
}

static void on_open_bp() {
  ui_winpop();
  ui_winpop();
  main_load_blueprint(C.bp);
}

static void blueprint_edit() { main_ask_for_save_and_proceed(on_open_bp); }

void win_bpdetail_update() {
  update_layout();

  bool is_local = C.bp->steam_id == 0;
  C.btn_publish.disabled = !is_steam_on() || !is_local;
  C.btn_edit_title.disabled = !is_local;
  C.btn_open_steam.disabled = is_local;

  if (IsKeyPressed(KEY_ESCAPE)) {
    ui_winpop();
    return;
  }

  if (IsKeyPressed(KEY_R)) {
    blueprint_rot(C.bp);
    return;
  }

  if (IsKeyPressed(KEY_ENTER)) {
    blueprint_paste(C.bp);
    play_sound_click();
    ui_winpop();
    ui_winpop();
    blueprint_store_save(C.store);
    return;
  }

  if (IsKeyPressed(KEY_F2) && is_local) {
    text_modal_open(on_rename_accept, NULL, C.bp->name);
    return;
  }

  if (btn_update(&C.btn_load)) {
    blueprint_edit();
    return;
  }

  if (btn_update(&C.btn_close)) {
    ui_winpop();
    return;
  }

  if (btn_update(&C.btn_use)) {
    blueprint_paste(C.bp);
    play_sound_click();
    ui_winpop();
    ui_winpop();
    blueprint_store_save(C.store);
    return;
  }

  if (btn_update(&C.btn_open_steam)) {
#ifdef WITH_STEAM
    steam_open_overlay_item(C.bp->steam_id);
#endif
    return;
  }

  if (btn_update(&C.btn_rotate)) {
    blueprint_rot(C.bp);
    return;
  }
  if (btn_update(&C.btn_copy)) {
    blueprint_copy_to_clipboard(C.bp);
    return;
  }

  if (btn_update(&C.btn_delete)) {
    assert(C.bp);
    if (!blueprint_can_delete(C.bp)) {
      win_msg_open_text("Blueprint can't be deleted while being editted.",
                        NULL);
      return;
    }

    dialog_open(TextFormat("Delete blueprint \"%s\"?", C.bp->name), "Delete",
                "Cancel", NULL, on_confirm_delete);
    return;
  }

  if (btn_update(&C.btn_publish)) {
    do_publish();
    return;
  }

  if (btn_update(&C.btn_edit_title)) {
    text_modal_open(on_rename_accept, NULL, C.bp->name);
    return;
  }
}

static void drawbg(Rectangle r) {
  Color bg = {0, 0, 0, 100};
  DrawRectangleRec(r, bg);
}

void win_bpdetail_draw() {
  draw_win(C.modal, "BLUEPRINT");
  label_set_text(&C.lab_name, C.bp->name ? C.bp->name : "(Unnamed)");
  Texture sprites = ui_get_sprites();
  // drawbg(C.sep);
  btn_draw_text_primary(&C.btn_use, 2, "PASTE");
  btn_draw_text(&C.btn_close, 2, "CLOSE");
  btn_draw_icon(&C.btn_edit_title, 2, sprites, rect_rename);
  btn_draw_icon(&C.btn_delete, 2, sprites, rect_trash);
  btn_draw_icon(&C.btn_publish, 2, sprites, rect_publish);
  btn_draw_icon(&C.btn_open_steam, 2, sprites, rect_steam);
  btn_draw_icon(&C.btn_load, 2, sprites, rect_open);
  btn_draw_icon(&C.btn_copy, 2, sprites, rect_copy);

  drawbg(C.lab_name.hitbox);
  drawbg(C.lab_author.hitbox);
  // drawbg(C.lab_props.hitbox);
  label_draw_centered(&C.lab_name);
  label_draw_centered(&C.lab_author);
  // label_draw_centered(&C.lab_props);

  Blueprint* bp = C.bp;
  if (bp && bp->thumbnail.id) {
    blueprint_draw(&C.btn_thumb, C.bp, 2);
  }
  btn_draw_icon(&C.btn_rotate, 2, sprites, rect_rot);

  if (ui_get_window() == WINDOW_BPDETAIL) {
    btn_draw_legend(&C.btn_use, 2,
                    "Paste blueprint (ENTER or RIGHT CLICK in inventory)");
    btn_draw_legend(&C.btn_rotate, 2, "Rotate thumbnail (R)");
    btn_draw_legend(&C.btn_copy, 2, "Copy blueprint's image to clipboard");
    btn_draw_legend(&C.btn_delete, 2, "Delete blueprint");
    if (C.bp->steam_id == 0) {
      btn_draw_legend(&C.btn_load, 2, "Edit blueprint's image");
    } else {
      btn_draw_legend(&C.btn_load, 2, "Load blueprint's image");
    }
    btn_draw_legend(&C.btn_edit_title, 2, "Rename blueprint (F2)");
    btn_draw_legend(&C.btn_publish, 2, "Publish on steam workshop");
    btn_draw_legend(&C.btn_open_steam, 2, "See item's page on workshop");
  }
}

void win_bpdetail_on_close() {
  if (C.delete_on_close) {
    C.delete_on_close = false;
    blueprint_store_rm(C.store, C.idx);
    blueprint_store_save(C.store);
    msg_add("Blueprint deleted.", 4);
  }
}
