#include "win_clipboard.h"

#include "layout.h"
#include "msg.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"
#include "win_main.h"

static struct {
  Layout* layout;
  Rectangle modal;
  Textbox tb;
  Btn btn_cancel;

  bool pending_load;
  unsigned char* pending_png; /* owned here, freed once decoded */
  int pending_png_len;
} C = {0};

static void update_layout() {
  Layout* l = C.layout;
  layout_update_offset(l);
  C.modal = layout_rect(l, "window");
  C.btn_cancel.hitbox = layout_rectb(l, "btn_cancel");
  textbox_set_box(&C.tb, layout_rectb(l, "frame"));
}

void win_clipboard_init() {
  C.layout = easy_load_layout("web_clipboard");
  textbox_init(&C.tb);
  textbox_set_content(&C.tb,
                      "To load an image:\n"
                      "`*` Drag a local PNG\n"
                      "`*` Ctrl+V (Cmd+V on Mac) an image\n"
                      "`*` Ctrl+V an image address (text)\n",
                      NULL);
}

void win_clipboard_open() {
  ui_winpush(WINDOW_CLIPBOARD);
  update_layout();
}

/* Perform a load requested from outside the frame loop. */
static bool consume_pending_load(void) {
  if (!C.pending_load) {
    return false;
  }

  C.pending_load = false;
  if (C.pending_png != NULL) {
    win_main_load_image_from_bytes(C.pending_png, C.pending_png_len);
    return true;
    free(C.pending_png);
    C.pending_png = NULL;
    C.pending_png_len = 0;
  }
  return false;
}

static bool check_file_drop() {
  if (!IsFileDropped()) {
    return false;
  }
  FilePathList path_list = LoadDroppedFiles();
  if (path_list.count > 0) {
    win_main_load_image_from_path(path_list.paths[0]);
  } else {
    return false;
  }
  UnloadDroppedFiles(path_list);
  return true;
}

void win_clipboard_update() {
  if (check_file_drop()) {
    msg_add("Loaded dragged image", 4);
    ui_winpop();
    return;
  };
  if (consume_pending_load()) {
    msg_add("Loaded clipboard image", 4);
    ui_winpop();
    return;
  };
  textbox_update(&C.tb);
  bool key_escape = IsKeyPressed(KEY_ESCAPE);
  if (btn_update(&C.btn_cancel) || key_escape) {
    ui_winpop();
    return;
  }
}

void win_clipboard_draw() {
  draw_win(C.modal, "Load from clipboard");
  btn_draw_text(&C.btn_cancel, "Cancel");
  textbox_draw(&C.tb);
}

bool win_clipboard_can_paste() { return ui_wintop() == WINDOW_CLIPBOARD; }

void win_clipboard_queue_paste(const unsigned char* data, int len) {
  if (!win_clipboard_can_paste()) {
    return;
  }
  free(C.pending_png);
  C.pending_png = malloc(len);
  if (C.pending_png == NULL) {
    C.pending_png_len = 0;
    return;
  }
  memcpy(C.pending_png, data, len);
  C.pending_png_len = len;
  C.pending_load = true;
}

