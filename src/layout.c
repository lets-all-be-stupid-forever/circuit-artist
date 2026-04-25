#include "layout.h"

#include "assert.h"
#include "paths.h"
#include "raylib.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utils.h"

static Rectangle layout_rect_raw(Layout* l, const char* id) {
  return shget(l->dict, id);
}

Rectangle layout_rect(Layout* l, const char* id) {
  return roff(l->off, layout_rect_raw(l, id));
}

void layout_update_offset(Layout* l) {
  Rectangle win = layout_rect_raw(l, "window");
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int ww = win.width;
  int wh = win.height;
  int x0 = (sw - ww) / 2;
  int y0 = (sh - wh) / 2;
  l->off = (Vector2){x0, y0};
}

static bool match(const char* a, const char* b) { return strcmp(a, b) == 0; }

Layout* parse_layout(const char* fname) {
  Layout* p = calloc(1, sizeof(Layout));
  char* txt = LoadFileText(fname);
  char tmp1[500];
  char tmp2[500];
  int c = 0;
  char* id = NULL;
  Rectangle r;
  bool inside_rect;
  bool is_rect = false;
  while (true) {
    while (txt[c] == ' ' || txt[c] == '\n') c++;
    if (txt[c] == '\0') break;
    if (txt[c] == '<') {
      c++;
      if (txt[c] == '/') {
        c++;
        int k = c;
        while (txt[c] != '>') {
          tmp1[c - k] = txt[c];
          c++;
        }
        tmp1[c - k] = '\0';
        if (match(tmp1, "svg")) {
          break;
        }
      } else {
        int k = c;
        while (txt[c] != ' ' && txt[c] != '/') {
          tmp1[c - k] = txt[c];
          c++;
        }
        tmp1[c - k] = '\0';
        r = (Rectangle){0};
        is_rect = match(tmp1, "rect");
      }
      continue;
    }
    if (txt[c] == '/') {
      assert(txt[c + 1] == '>');
      c += 2;
      if (!is_rect) {
        continue;
      }
      if (!id) {
        id = clone_string("window");
      }
      shput(p->dict, id, r);
      id = NULL;
      continue;
    }
    if (txt[c] == '>') {
      c++;
      continue;
    }
    // attr
    {
      int k = c;
      while (txt[c] != '=') {
        tmp1[c - k] = txt[c];
        c++;
      }
      tmp1[c - k] = '\0';
      c++;
      c++;
      k = c;
      while (txt[c] != '"') {
        if (is_rect) tmp2[c - k] = txt[c];
        c++;
      }
      if (is_rect) tmp2[c - k] = '\0';
      c++;
      if (!is_rect) continue;
      if (match(tmp1, "id")) {
        assert(id == NULL);
        if (is_rect) {
          id = clone_string(tmp2);
        }
      } else if (match(tmp1, "width")) {
        r.width = atoi(tmp2);
      } else if (match(tmp1, "height")) {
        r.height = atoi(tmp2);
      } else if (match(tmp1, "x")) {
        r.x = atoi(tmp2);
      } else if (match(tmp1, "y")) {
        r.y = atoi(tmp2);
      }
    }
  }
  UnloadFileText(txt);

  if (false) {
    int n = shlen(p->dict);
    for (int i = 0; i < n; i++) {
      printf("rect[%s] = (%f %f %f %f)\n", p->dict[i].key, p->dict[i].value.x,
             p->dict[i].value.y, p->dict[i].value.width,
             p->dict[i].value.height);
    }
  }

  return p;
}

void layout_free(Layout* p) {
  int n = shlen(p->dict);
  for (int i = 0; i < n; i++) {
    free(p->dict[i].key);
  }
  shfree(p->dict);
  free(p);
}

Layout* easy_load_layout(const char* win_name) {
  char* layout_path = get_asset_path(TextFormat("layout/%s.svg", win_name));
  Layout* layout = parse_layout(layout_path);
  free(layout_path);
  return layout;
}
