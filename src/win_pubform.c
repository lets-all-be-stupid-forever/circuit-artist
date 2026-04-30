#include "win_pubform.h"

#include "fs.h"
#include "layout.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"
#include "win_mtext.h"
#include "win_progress.h"
#include "wtext.h"

static struct {
  Layout* layout;
  Rectangle modal;
  Label lab_title;
  Label lab_desc;
  Rectangle rect_preview;
  Btn btn_submit;
  Btn btn_cancel;
  Btn btn_edit_title;
  Btn btn_edit_desc;
  Textbox tb_desc;
  Label tb_title;
  void (*on_cancel)();
  double upload_counter;
  char* thumb_path;
  char* folder;
  Texture2D thumb_tex;
} C = {0};

static void update_layout() {
  Layout* l = C.layout;
  layout_update_offset(l);
  C.modal = layout_rect(l, "window");
  C.btn_cancel.hitbox = layout_rect(l, "btn_cancel");
  C.btn_submit.hitbox = layout_rect(l, "btn_submit");
  C.btn_edit_title.hitbox = layout_rect(l, "btn_edit_title");
  C.btn_edit_desc.hitbox = layout_rect(l, "btn_edit_desc");
  C.lab_desc.hitbox = layout_rect(l, "lab_desc");
  C.lab_title.hitbox = layout_rect(l, "lab_title");
  C.rect_preview = layout_rect(l, "rect_preview");
  textbox_set_box(&C.tb_desc, layout_rect(l, "tb_desc"));
  // textbox_set_box(&C.tb_title, layout_rect(l, "tb_title"));
  C.tb_title.hitbox = layout_rect(l, "tb_title");
}

void win_pubform_init() {
  C.layout = easy_load_layout("pubform");
  label_set_text(&C.lab_desc, "Description");
  label_set_text(&C.lab_title, "Steam Name");
}

static void reset_fields() {
  if (C.thumb_path) {
    free(C.thumb_path);
    UnloadTexture(C.thumb_tex);
    C.thumb_path = NULL;
    C.thumb_tex = (Texture2D){0};
  }
  free(C.folder);
  C.folder = NULL;
}

static void set_folder(const char* path) { C.folder = clone_string(path); }

static void set_thumb_path(const char* path) {
  if (path) {
    C.thumb_path = abs_path(path);
    C.thumb_tex = LoadTexture(C.thumb_path);
  }
}

void win_pubform_open(PubformParams params) {
  ui_winpush(WINDOW_PUBFORM);
  reset_fields();
  // TODO: it can crash if ppl put things like !img here!
  textbox_set_content(&C.tb_desc, params.default_desc, NULL);
  label_set_text(&C.tb_title, params.default_title);
  set_thumb_path(params.default_thumbnail_path);
  set_folder(params.folder);
  update_layout();
}

static void on_rename_accept(void* ctx, const char* txt) {
  // textbox_set_content(&C.tb_title, txt, NULL);
  label_set_text(&C.tb_title, txt);
}

static void on_rename_desc_accept(void* ctx, const char* txt) {
  textbox_set_content(&C.tb_desc, txt, NULL);
}

static bool progress_update(void* ctx) {
  bool done = steam_upload_update(ctx);
  if (done) {
    steam_upload_free(ctx);
  }
  return done;
}

static void launch_publish() {
  win_progress_set_text("Uploading");
  C.upload_counter = 0;
  ProgressCtx pc = {0};
  char* tags[2] = {"blueprint", NULL};

  pc.ctx =
      steam_upload_item(C.folder, "Change note", C.tb_title.txt, C.tb_desc.text,
                        C.thumb_path, 1, (const char**)tags);
  pc.update = progress_update;
  win_progress_open(pc);
}

void win_pubform_update() {
  update_layout();
  bool key_escape = IsKeyPressed(KEY_ESCAPE);
  C.btn_edit_desc.hidden = true; /* Disabling it for now for simplicity*/
  textbox_update(&C.tb_desc);
  // textbox_update(&C.tb_title);
  if (btn_update(&C.btn_edit_title)) {
    // text_modal_open(on_rename_accept, NULL, C.tb_title.text);
    text_modal_open(on_rename_accept, NULL, C.tb_title.txt);
    return;
  }
  if (btn_update(&C.btn_edit_desc)) {
    win_mtext_open(on_rename_desc_accept, NULL, C.tb_desc.text);
    return;
  }
  if (key_escape || btn_update(&C.btn_cancel)) {
    ui_winpop();
  }
  if (btn_update(&C.btn_submit)) {
    ui_winpop();
    launch_publish();
  }
}

void win_pubform_draw() {
  draw_win(C.modal, "PUBLISH TO STEAM");
  DrawRectangleRec(C.rect_preview, RED);
  Texture sprites = ui_get_sprites();
  btn_draw_text(&C.btn_cancel, 2, "CANCEL");
  btn_draw_text_primary(&C.btn_submit, 2, "PUBLISH");
  btn_draw_icon(&C.btn_edit_title, 2, sprites, rect_rename);
  btn_draw_icon(&C.btn_edit_desc, 2, sprites, rect_rename_desc);
  // textbox_draw(&C.tb_title);
  // label_draw(&C.tb_title);
  // textbox_draw(&C.tb_desc);
  Color c = {0, 0, 0, 150};
  DrawRectangleRec(C.tb_title.hitbox, c);
  label_draw(&C.tb_title);
  label_draw(&C.lab_title);

  if (C.thumb_tex.width > 0) {
    Rectangle src = {
        0,
        0,
        C.thumb_tex.width,
        C.thumb_tex.height,
    };
    Rectangle dst = C.rect_preview;
    DrawTexturePro(C.thumb_tex, src, dst, (Vector2){0, 0}, 0, WHITE);
  }

  if (ui_get_window() == WINDOW_PUBFORM) {
    btn_draw_legend(&C.btn_edit_title, 2, "Change name");
    btn_draw_legend(&C.btn_edit_desc, 2, "Change description");
  }
}

void win_pubform_on_close() { reset_fields(); }
