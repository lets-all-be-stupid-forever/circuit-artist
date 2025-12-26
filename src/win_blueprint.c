#include "win_blueprint.h"

#include "assert.h"
#include "font.h"
#include "math.h"
#include "msg.h"
#include "rlgl.h"
#include "stb_ds.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"
#include "wmain.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

typedef struct {
  int score;
  int idx;
} ScoreIdx;

static struct {
  char** files; /* path to png files */
  char** names; /* real fname (searcheable) */
  int* flag;
  int ncells;
  int nr;
  int nc;
  Rectangle win;
  int iscore;
  ScoreIdx* score_idx;
  int nscore;
  int scroll;
  Editbox ebox;
  Btn btn[500];
} C = {0};

static int getsel() {
  if (C.nscore == 0) return -1;
  return C.score_idx[C.iscore].idx;
}

static int get_tot_rows() {
  int n = arrlen(C.files);
  int tot_rows = n / C.nc;
  if (tot_rows < C.nr) tot_rows = C.nr;
  return tot_rows;
}

static void reset_files() {
  int n = arrlen(C.files);
  assert(arrlen(C.names) == n);
  for (int i = 0; i < n; i++) {
    free(C.files[i]);
    free(C.names[i]);
  }
  arrfree(C.files);
  arrfree(C.names);
  arrfree(C.flag);
  arrfree(C.score_idx);
  C.names = NULL;
  C.flag = NULL;
  C.score_idx = NULL;
  C.files = NULL;
}

static int get_max_scroll() {
  int n = arrlen(C.files);
  int rows = (n - 1) / C.nc + 1;
  if (rows <= 0) rows = 1;
  int nr = C.nr; /*rows being shown*/
  int max_scroll = rows - C.nr;
  if (max_scroll < 0) max_scroll = 0;
  return max_scroll;
}

static void update_layout() {
  int bw = 160;
  int sh = GetScreenHeight();
  int sw = GetScreenWidth();
  int bh = 20;  // l(non)

  int nc = 6;
  int nr = 31;  // 20;  // 20;
  C.nr = nr;
  C.nc = nc;
  C.ncells = nc * nr;

  int header = 40;
  int footer = 40;
  int win_w = bw * nc;
  int win_h = bh * nr + footer + header;
  C.win = (Rectangle){
      .x = (sw - win_w) / 2,
      .y = (sh - win_h) / 2,
      .width = win_w,
      .height = win_h,
  };
  C.ebox.hitbox = (Rectangle){
      C.win.x + 2,
      C.win.y + 2,
      bw,
      bh,
  };
  for (int c = 0; c < C.nc; c++) {
    for (int r = 0; r < C.nr; r++) {
      int i = c * C.nr + r;
      C.btn[i].hitbox = (Rectangle){
          C.win.x + c * bw,
          C.win.y + header + r * bh,
          bw,
          bh,
      };
    }
  }
}

// Comparison function for sorting files by name (case insensitive)
static int compare_files(const void* a, const void* b) {
  int ia = *(const int*)a;
  int ib = *(const int*)b;
  return strcasecmp(C.names[ia], C.names[ib]);
}

static int clipi(int a, int vmin, int vmax) {
  if (a < vmin) a = vmin;
  if (a > vmax) a = vmax;
  return a;
}

static void scroll_goto(int ib) {
  int smax = get_max_scroll();
  int r = ib % get_tot_rows();
  C.scroll = clipi(r, 0, smax);
}

static int compare_score_idx(const void* a, const void* b) {
  const ScoreIdx* sa = a;
  const ScoreIdx* sb = b;
  if (sa->score == sb->score) {
    if (sa->idx < sb->idx) return -1;
    if (sa->idx > sb->idx) return 1;
    return 0;
  }
  if (sa->score < sb->score) return 1;
  return -1;
}

static void update_query() {
  const char* q = C.ebox.txt;
  int n = arrlen(C.files);
  if (strlen(q) == 0) {
    for (int i = 0; i < n; i++) {
      C.flag[i] = 1;
    }
    C.nscore = 0;
  } else {
    // Fuzzy search query over C.names
    int nscore = 0;
    for (int i = 0; i < n; i++) {
      if (fuzzy_match_simple(C.names[i], q)) {
        C.flag[i] = 1;
        C.score_idx[nscore++] = (ScoreIdx){
            .idx = i,
            .score = fuzzy_score(C.names[i], q),
        };
      } else {
        C.flag[i] = 0;
      }
    }

    C.nscore = nscore;
    C.iscore = 0;
    if (nscore > 0) {
      qsort(C.score_idx, nscore, sizeof(ScoreIdx), compare_score_idx);
      scroll_goto(getsel());
    }
  }
}

static void update_files() {
  reset_files();
  C.files = find_png_files("../solutions/");
  int n = arrlen(C.files);
  for (int i = 0; i < n; i++) {
    arrput(C.names, extract_filename_no_ext(C.files[i]));
  }
  C.nscore = 0;
  // Sort files by name
  if (n > 0) {
    // Create index array for sorting
    int* indices = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }

    // Sort indices based on names
    qsort(indices, n, sizeof(int), compare_files);

    // Create sorted arrays
    char** sorted_files = NULL;
    char** sorted_names = NULL;
    for (int i = 0; i < n; i++) {
      arrput(sorted_files, C.files[indices[i]]);
      arrput(sorted_names, C.names[indices[i]]);
      arrput(C.flag, 0);
      arrput(C.score_idx, (ScoreIdx){0});
    }

    // Replace with sorted arrays (don't free the strings, just the arrays)
    arrfree(C.files);
    arrfree(C.names);
    C.files = sorted_files;
    C.names = sorted_names;

    free(indices);
  }
}

void win_blueprint_open() {
  ui_winpush(WINDOW_BLUEPRINT);
  update_files();
  update_layout();
  editbox_init(&C.ebox);
  update_query();
}

static void use_blueprint(int ib) {
  assert(ib < arrlen(C.files));
  assert(ib >= 0);
  ui_winpop();
  const char* fname = C.files[ib];
  const char* name = C.names[ib];
  if (!FileExists(fname)) {
    msg_add(TextFormat("Couldn't paste: File no longer exists: %s", name), 3);
    return;
  }
  main_paste_file(fname, 0);
}

static void scroll_shift(int z) {
  int scroll = C.scroll - z;
  int smax = get_max_scroll();
  if (scroll < 0) scroll = 0;
  if (scroll >= smax) scroll = smax;
  C.scroll = scroll;
}

static void update_wheel() {
  int n = arrlen(C.files);
  float wheel = GetMouseWheelMove();
  if (fabs(wheel) > 1e-3) {
    int z = wheel > 0 ? 1 : -1;
    scroll_shift(z);
  }
}

void sel_next(bool prv) {
  if (C.nscore == 0) return;
  if (prv) {
    C.iscore = (C.iscore + C.nscore - 1) % C.nscore;
  } else {
    C.iscore = (C.iscore + 1) % C.nscore;
  }
  scroll_goto(getsel());
}

void win_blueprint_update() {
  update_layout();
  if (editbox_update(&C.ebox)) {
    update_query();
  }

  if (IsKeyPressed(KEY_TAB)) {
    bool prv = IsKeyDown(KEY_LEFT_SHIFT);
    sel_next(prv);
  }

  if (IsKeyPressed(KEY_UP)) {
    scroll_shift(1);
  }
  if (IsKeyPressed(KEY_DOWN)) {
    scroll_shift(-1);
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    ui_winpop();
    return;
  }

  if (IsKeyPressed(KEY_ENTER)) {
    if (C.nscore > 0) use_blueprint(getsel());
    return;
  }
  update_wheel();

  int n = arrlen(C.files);
  Vector2 pos = GetMousePosition();
  int tot_rows = get_tot_rows();
  for (int c = 0; c < C.nc; c++) {
    int off = c * tot_rows + C.scroll;
    for (int r = 0; r < C.nr; r++) {
      int ib = off + r;
      if (ib < n) {
        if (btn_update(&C.btn[c * C.nr + r])) {
          use_blueprint(ib);
        }
      }
    }
  }
}

void win_blueprint_draw() {
  int sh = GetScreenHeight();
  int sw = GetScreenWidth();
  Color bg = BLACK;
  bg.a = 150;
  DrawRectangle(0, 0, sw, sh, bg);
  DrawRectangleRec(C.win, BLACK);
  font_draw_texture("bp:", 20, 20, WHITE);
  int n = arrlen(C.files);
  editbox_draw(&C.ebox);
  rlPushMatrix();
  rlScalef(2, 2, 1);
  int lh = get_font_line_height();

  int sel = getsel();
  int tot_rows = get_tot_rows();
  for (int c = 0; c < C.nc; c++) {
    int off = c * tot_rows + C.scroll;
    for (int r = 0; r < C.nr; r++) {
      int i = c * C.nr + r;
      int x = C.btn[i].hitbox.x / 2;
      int y = C.btn[i].hitbox.y / 2;
      int w = C.btn[i].hitbox.width / 2;
      int h = C.btn[i].hitbox.height / 2;
      bool hover = C.btn[i].hover;
      int ib = off + r;
      Color clr = PINK;
      if (ib < n) {
        if (hover) {
          clr = GREEN;
          clr.a = 100;
          ui_set_cursor(MOUSE_POINTER);
        }
        if (sel == ib) clr = YELLOW;
        DrawRectangle(x, y, w, h, clr);
        int oy = (h - lh) / 2;
        Color c = BLUE;
        if (C.flag[ib] == 0) c.a = 30;
        font_draw_texture(C.names[ib], x + 2, y + oy, c);
      }
    }
  }
  rlPopMatrix();
}

void win_blueprint_init() {}
