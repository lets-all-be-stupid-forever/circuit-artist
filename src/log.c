#include "log.h"

#include "buffer.h"
#include "stb_ds.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

/*
 * Returns an array of strings (allocated with malloc).
 */
static char** split_lines(const char* txt) {
  char** ret = NULL;
  int n = strlen(txt);
  int i = 0;
  while (i < n) {
    int i0 = i;
    while (txt[i] != '\n' && txt[i] != '\0') i++;
    int llen = i - i0 + 1;
    char* tmp = (char*)malloc(llen);
    for (int k = i0; k < i; k++) {
      tmp[k - i0] = txt[k];
    }
    tmp[i - i0] = '\0';
    arrput(ret, tmp);
    /* jumps the newline*/
    i++;
  }
  return ret;
}

char* log_get_line(SimpleLog* l, int nb) {
  int cap = l->cap;
  return l->lines[(l->first + nb) % cap];
}

int log_get_num_lines(SimpleLog* l) { return l->num_lines; }

void log_init(SimpleLog* l) {
  l->cap = 100;
  l->lines = calloc(l->cap, sizeof(char*));
  l->num_lines = 0;
}

void log_free(SimpleLog* l) {
  for (int i = 0; i < l->cap; i++) {
    free(l->lines[i]);
  }
  free(l->lines);
}

static void log_add_line(SimpleLog* l, char* line) {
  if (l->num_lines == l->cap) {
    free(l->lines[l->first]);
    l->lines[l->first] = NULL;
    l->first = (l->first + 1) % l->cap;
    l->num_lines--;
  }
  int idx = (l->first + l->num_lines++) % l->cap;
  assert(l->lines[idx] == NULL);
  l->lines[idx] = line;
}

/* Doesnt take ownership of input string. */
void log_add_text(SimpleLog* l, const char* txt) {
  /* 1. separate by lines */
  /* 2. append each line to the log */
  char** lines = split_lines(txt);
  int nl = arrlen(lines);
  for (int i = 0; i < nl; i++) {
    log_add_line(l, lines[i]);
  }
  arrfree(lines);
}

// what's the use case here?
// Error message
// debug message

static struct {
  SimpleLog log;
  char* generated_text;
  Textbox tb;
  Btn btn_close;
  Rectangle modal;
} C = {0};

void win_log_init() {
  textbox_init(&C.tb);

  log_init(&C.log);
}

static void update_layout() {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int bw = 12 * 35 * 2;
  int s = 2;
  int total_w = bw + 2 * 2;
  int x = (sw - total_w) / 2;
  int bh = 9 * 35 * s;
  int total_h = (bh + 35 * 1 + 35 * 1);
  int pad = 10 * s;
  while (total_h + 2 * pad + 10 > sh) {
    total_h -= 35 * s;
    bh -= 35 * s;
  }
  int y = (sh - total_h) / 2;
  int yy = y + 35 * 1;
  C.modal = (Rectangle){x - pad, y - pad, total_w + 2 * pad, total_h + 2 * pad};
  Rectangle box = {x + 2 * s, yy, bw, bh - 4 * s};
  Rectangle box2 = box;
  int bsize = 4 * 35 * s;
  C.btn_close.hitbox = (Rectangle){
      box.x + box.width - bsize,
      box.y + box.height + 8 * 2,
      bsize,
      17 * 2,
  };
  textbox_set_box(&C.tb, box2);
}

void win_log_update() {
  update_layout();
  if (btn_update(&C.btn_close) || IsKeyPressed(KEY_ESCAPE)) {
    ui_winpop();
    return;
  }
  textbox_update(&C.tb);
}

void win_log_draw() {
  draw_win(C.modal, "LOG");
  textbox_draw(&C.tb);
  btn_draw_text(&C.btn_close, ui_get_scale(), "CLOSE");
}

static void win_log_build_text() {
  if (C.generated_text) free(C.generated_text);
  SimpleLog* l = &C.log;
  int n = log_get_num_lines(l);
  size_t size = 0;
  for (int i = 0; i < n; i++) {
    const char* line = log_get_line(l, i);
    size += strlen(line) + 1;
  }
  if (size > 0) {
    C.generated_text = malloc(size);
    char* p = C.generated_text;
    for (int i = 0; i < n; i++) {
      const char* line = log_get_line(l, i);
      int m = strlen(line);
      for (int j = 0; j < m; j++) {
        *p++ = line[j];
      }
      if (i != n - 1) *p++ = '\n';
    }
    *p++ = '\0';
  } else {
    C.generated_text = malloc(1);
    C.generated_text[0] = '\0';
  }
  textbox_set_content(&C.tb, C.generated_text, NULL);
  textbox_scroll_to_bottom(&C.tb);
}

void win_log_open() {
  ui_winpush(WINDOW_LOG);
  update_layout();
  win_log_build_text();
}

void lua_log_text(const char* txt) { log_add_text(&C.log, txt); }
