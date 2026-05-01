#include "win_pubform.h"

#include "fs.h"
#include "layout.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"
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
  MultiLineEdit tb_desc;
  LineEdit tb_title;
  Textbox tb_legal;
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
  C.lab_desc.hitbox = layout_rect(l, "lab_desc");
  C.lab_title.hitbox = layout_rect(l, "lab_title");
  C.rect_preview = layout_rect(l, "rect_preview");
  mle_set_box(&C.tb_desc, layout_rect(l, "tb_desc"));
  // textbox_set_box(&C.tb_title, layout_rect(l, "tb_title"));
  C.tb_title.hitbox = layout_rect(l, "tb_title");
  textbox_set_box(&C.tb_legal, layout_rect(l, "tb_legal"));
}

void win_pubform_init() {
  C.layout = easy_load_layout("pubform");
  label_set_text(&C.lab_desc, "Description");
  label_set_text(&C.lab_title, "Title");
  lineedit_init(&C.tb_title);
  mle_init(&C.tb_desc);
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
  mle_set_text(&C.tb_desc, params.default_desc);
  lineedit_set_text(&C.tb_title, params.default_title);
  lineedit_set_focus(&C.tb_title, true);
  mle_set_focus(&C.tb_desc, false);
  set_thumb_path(params.default_thumbnail_path);
  set_folder(params.folder);
  update_layout();
}

static bool progress_update(void* ctx) {
  bool done = steam_upload_update(ctx);
  if (done) {
    steam_upload_free(ctx);
  }
  return done;
}

static void launch_publish() {
  win_progress_set_text("Uploading to Steam");
  C.upload_counter = 0;
  ProgressCtx pc = {0};
  char* tags[2] = {"blueprint", NULL};

  pc.ctx = steam_upload_item(
      C.folder, "Change note", lineedit_get_text(&C.tb_title),
      mle_get_text(&C.tb_desc), C.thumb_path, 1, (const char**)tags);
  pc.update = progress_update;
  win_progress_open(pc);
}

void win_pubform_update() {
  update_layout();
  textbox_update(&C.tb_legal);
  textbox_set_content(&C.tb_legal,
                      "By submitting this blueprint, you agree to the `Steam "
                      "Workshop terms of service.`",
                      NULL);
  bool key_escape = IsKeyPressed(KEY_ESCAPE);
  mle_update(&C.tb_desc);
  lineedit_update(&C.tb_title);

  if (IsKeyPressed(KEY_TAB)) {
    int of = -1;
    if (C.tb_title.focused) of = 1;
    if (C.tb_desc.focused) of = 2;
    int nf = -1;
    if (of == 1) nf = 2;
    if (of == 2) nf = 1;
    if (of == -1) nf = 1;
    lineedit_set_focus(&C.tb_title, nf == 1);
    mle_set_focus(&C.tb_desc, nf == 2);
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
  lineedit_draw(&C.tb_title);
  mle_draw(&C.tb_desc);
  label_draw(&C.lab_desc);
  label_draw(&C.lab_title);
  textbox_draw(&C.tb_legal);

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
}

void win_pubform_on_close() { reset_fields(); }
