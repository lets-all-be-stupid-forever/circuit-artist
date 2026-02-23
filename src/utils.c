#include "utils.h"

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "common.h"
#include "font.h"
#include "fs.h"
#include "paths.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "ui.h"

// Implementation of the stb_ds library.
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

// Does the same as strdup().
char* clone_string(const char* str) {
  size_t len = strlen(str) + 1;
  char* p = (char*)malloc(len);
  memmove(p, str, len);
  return p;
}

// Checks for left control or macos command key down
bool is_control_down() {
  return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_LEFT_SUPER);
}

// Descriptor for a frame pattern.
// It's used for drawing the frame of the drawing image and the border of
// windows/modals.
typedef struct {
  Texture2D tex;
  Rectangle frame_left;
  Rectangle frame_up;
  Rectangle frame_down;
  Rectangle frame_right;
  Rectangle corner;
} FramePatternDesc;

// Draws a tiled frame around a rectangle region.
// `s` is the scale of the frame.
static void draw_tiled_frame(int s, FramePatternDesc pd,
                             Rectangle inner_content) {
  // Idea:
  // 1. Draw each side
  // 2. Draw corners on top

  int cx = inner_content.x;
  int cy = inner_content.y;
  int ch = inner_content.height;
  int cw = inner_content.width;
  int th = pd.frame_left.height;
  int tw = pd.frame_left.width;

  BeginScissorMode(cx - s * tw, cy - s * tw, cw + 2 * s * tw, ch + 2 * s * tw);

  int nx = (cw + s * tw - 1) / (s * tw);
  int ny = (ch + s * th - 1) / (s * th);
  rlPushMatrix();
  rlTranslatef(cx, cy, 0);
  rlScalef(s, s, 1);
  rlTranslatef(0, 0, 0);
  for (int ix = 0; ix < nx; ix++) {
    int x = ix * th;
    Rectangle dest = {
        x,
        -tw,
        th,
        tw,
    };
    float rotation = 0;
    Vector2 origin = {0};  //{tw / 2.0, th / .0};
    // Rectangle src = frame;
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_up, dest, origin, rotation, tint);
  }

  // LEft side
  for (int iy = 0; iy < ny; iy++) {
    int y = iy * th;
    Rectangle dest = {
        -tw,
        y,
        tw,
        th,
    };
    float rotation = 0;
    Vector2 origin = {0};  //{tw / 2.0, th / .0};
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_left, dest, origin, rotation, tint);
  }

  // Bottom
  for (int ix = 0; ix < nx; ix++) {
    int x = ix * th;
    Rectangle dest = {
        x,
        ch / s,
        th,
        tw,
    };
    float rotation = 0;
    Vector2 origin = {0};
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_down, dest, origin, rotation, tint);
  }

  // Right side
  for (int iy = 0; iy < ny; iy++) {
    int y = iy * th;
    Rectangle dest = {
        cw / s,
        y,
        tw,
        th,
    };
    float rotation = 0;
    Vector2 origin = {0};  //{tw / 2.0, th / .0};
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_right, dest, origin, rotation, tint);
  }

  DrawTexturePro(pd.tex, pd.corner, (Rectangle){-tw, -tw, tw, tw}, (Vector2){0},
                 0, WHITE);
  DrawTexturePro(pd.tex, pd.corner, (Rectangle){cw / s, -tw, tw, tw},
                 (Vector2){0}, 0, WHITE);
  DrawTexturePro(pd.tex, pd.corner, (Rectangle){-tw, ch / s, tw, tw},
                 (Vector2){0}, 0, WHITE);
  DrawTexturePro(pd.tex, pd.corner, (Rectangle){cw / s, ch / s, tw, tw},
                 (Vector2){0}, 0, WHITE);

  rlPopMatrix();
  EndScissorMode();
}

// Draws a tiled pattern in the whole screen. The pattern is the `src` region
// within the texture `tex`, which is repeated on screen with scaling of `s`.
// Used for drawing UI background.
void draw_tiled_screen(int s, Texture2D tex, Rectangle src) {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  draw_tiled_rect(s, tex, src, (Rectangle){0, 0, sw, sh});
#if 0
  rlPushMatrix();
  rlScalef(s, s, 1);
  int tilew = src.width;
  int tileh = src.height;
  int screenw = (GetScreenWidth() + s - 1) / s;
  int screenh = (GetScreenHeight() + s - 1) / s;
  int nx = (screenw + tilew - 1) / tilew;
  int ny = (screenh + tileh - 1) / tileh;
  for (int ty = 0; ty < ny; ty++) {
    for (int tx = 0; tx < nx; tx++) {
      int x = tx * tilew;
      int y = ty * tilew;
      // RLAPI void DrawTexturePro(Texture2D texture, Rectangle source,
      // Rectangle dest, Vector2 origin, float rotation, Color tint); // Draw a
      // part of a texture defined by a rectangle with 'pro' parameters
      Rectangle dest = {
          x,
          y,
          tilew,
          tileh,
      };
      Vector2 origin = {0};
      float rotation = 0;
      Color tint = WHITE;
      DrawTexturePro(tex, src, dest, origin, rotation, tint);
    }
  }
  rlPopMatrix();
#endif
}

void draw_tiled_rect(int s, Texture2D tex, Rectangle src, Rectangle dst) {
  BeginScissorMode(dst.x, dst.y, dst.width, dst.height);
  rlPushMatrix();
  rlTranslatef(dst.x, dst.y, 0);
  rlScalef(s, s, 1);
  int tilew = src.width;
  int tileh = src.height;
  int dw = (dst.width + s - 1) / s;
  int dh = (dst.height + s - 1) / s;
  int nx = (dw + tilew - 1) / tilew;
  int ny = (dh + tileh - 1) / tileh;
  for (int ty = 0; ty < ny; ty++) {
    for (int tx = 0; tx < nx; tx++) {
      int x = tx * tilew;
      int y = ty * tileh;
      // RLAPI void DrawTexturePro(Texture2D texture, Rectangle source,
      // Rectangle dest, Vector2 origin, float rotation, Color tint); // Draw a
      // part of a texture defined by a rectangle with 'pro' parameters
      Rectangle dest = {
          x,
          y,
          tilew,
          tileh,
      };
      Vector2 origin = {0};
      float rotation = 0;
      Color tint = WHITE;
      DrawTexturePro(tex, src, dest, origin, rotation, tint);
    }
  }
  rlPopMatrix();
  EndScissorMode();
}

// Frame for windows.
static void draw_widget_frame_inv(Rectangle r) {
  int s = ui_get_scale();
  int x = r.x - 2 * s;
  int y = r.y - 2 * s;
  int w = r.width + 4 * s;
  int h = r.height + 4 * s;
  Color c0 = get_lut_color(COLOR_BTN0);  // darker
  Color c1 = get_lut_color(COLOR_BTN1);
  Color c2 = get_lut_color(COLOR_BTN2);  // lighter

  DrawRectangle(x, y, w - s, s, c1);                      // outter top
  DrawRectangle(x + s, y + s, w - 3 * s, s, c2);          // inner top
  DrawRectangle(x, y, s, h - s, c1);                      // outter left
  DrawRectangle(x + s, y + s, s, h - 3 * s, c2);          // inner left
  DrawRectangle(x + w - s, y, s, h, BLACK);               // outter right
  DrawRectangle(x + w - 2 * s, y + s, s, h - 2 * s, c0);  // inner right
  DrawRectangle(x + s, y + h - 2 * s, w - 2 * s, s, c0);  // inner bottom
  DrawRectangle(x, y + h - s, w - s, s, BLACK);           // outter bottom
}

// Title for framed windows
static void draw_title(Rectangle modal, const char* title) {
  rlPushMatrix();
  rlTranslatef(modal.x, modal.y, 0);
  int w = get_rendered_text_size(title).x;

  int th = 20;
  int lh = get_font_line_height();
  int offy = (th - lh) / 2 + 2;
  int offx = (modal.width / 2 - w) / 2;
  int s = 2;
  Color k = {21, 11, 3, 255};
  DrawRectangle(0, 0, modal.width, s * th, k);
  rlScalef(s, s, 1);
  font_draw_texture(title, offx, offy, WHITE);
  rlPopMatrix();
}

void draw_win(Rectangle r, const char* title) {
  Color bg = BLACK;
  bg.a = 150;
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  DrawRectangle(0, 0, sw, sh, bg);
  // draw_tiled_rect(ui_get_scale(), ui_get_sprites(), rect_bg_pattern, r);
  draw_tiled_rect(ui_get_scale(), ui_get_sprites(), rect_bg_pattern, r);
  // draw_widget_frame_inv(r);
  draw_default_tiled_frame(r);
  if (title) {
    draw_title(r, title);
  }
}

// Default hard coded tiled pattern.
void draw_default_tiled_screen() {
  draw_tiled_screen(ui_get_scale(), ui_get_sprites(), rect_bg_pattern);
}

// Default hard coded frame pattern.
void draw_default_tiled_frame(Rectangle inner_content) {
  FramePatternDesc pd = {
      .tex = ui_get_sprites(),
      .frame_left = (Rectangle){816, 16, 7, 12},
      .frame_up = (Rectangle){832, 16, 12, 7},
      .frame_right = (Rectangle){848, 16, 7, 12},
      .frame_down = (Rectangle){864, 16, 12, 7},
      .corner = (Rectangle){832, 0, 7, 7},
  };
  draw_tiled_frame(ui_get_scale(), pd, inner_content);
}

bool next_token(const char* str, int* ci, char sep, char* out) {
  int i = *ci;
  while (str[i] == sep && str[i] != '\0' && str[i] != '\n' && str[i] != '\r') {
    i++;
  }
  int i0 = i;
  while (str[i] != sep && str[i] != '\0' && str[i] != '\n' && str[i] != '\r') {
    i++;
  }
  int i1 = i;
  *ci = i1;
  if (i1 == i0) {
    return false;
  } else {
    for (int j = i0; j < i1; j++) {
      out[j - i0] = str[j];
    }
    out[i1 - i0] = '\0';
    return true;
  }
}

void print_matrix(const char* name, Matrix m) {
  printf("%s = \n", name);
  printf("%9.5f %9.5f %9.5f %9.5f\n", m.m0, m.m4, m.m8, m.m12);
  printf("%9.5f %9.5f %9.5f %9.5f\n", m.m1, m.m5, m.m9, m.m13);
  printf("%9.5f %9.5f %9.5f %9.5f\n", m.m2, m.m6, m.m10, m.m14);
  printf("%9.5f %9.5f %9.5f %9.5f\n", m.m3, m.m7, m.m11, m.m15);
  printf("\n");
}

void str_builder_init(str_builder_t* sb) {
  sb->cap = 100;
  sb->data = malloc(sb->cap);
  sb->size = 0;
}

void str_builder_add_raw(str_builder_t* sb, const char* txt) {
  int n = strlen(txt);
  while (n + sb->size + 2 > sb->cap) {
    sb->cap *= 2;
    sb->data = realloc(sb->data, sb->cap);
  }
  for (int i = 0; i < n + 1; i++) {
    sb->data[sb->size + i] = txt[i];
  }
  sb->size += n;
}

void str_builder_add(str_builder_t* sb, const char* fmt, ...) {
  static char buf[1024];
  va_list args;
  int result, n;
  va_start(args, fmt);
  result = vsnprintf(buf, 1024, fmt, args);
  va_end(args);
  n = result;
  if (n > 1024) n = 1024;
  str_builder_add_raw(sb, buf);
}

void str_builder_destroy(str_builder_t* sb) {
  free(sb->data);
  sb->data = NULL;
  sb->size = 0;
  sb->cap = 0;
}

sprite_t load_sprite_raw(const char* asset) {
  Texture tex = LoadTexture(asset);
  return (sprite_t){.tex = tex,
                    .region = (Rectangle){
                        0,
                        0,
                        tex.width,
                        tex.height,
                    }};
}

sprite_t load_sprite_asset(const char* asset) {
  Texture tex = load_texture_asset(asset);
  return (sprite_t){.tex = tex,
                    .region = (Rectangle){
                        0,
                        0,
                        tex.width,
                        tex.height,
                    }};
}

static inline bool is_bg(Color c) {
  return (c.r != 255 || c.g != 255 || c.b != 255);
}

Rectangle* parse_layout_asset(const char* asset) {
  Rectangle* out = NULL;
  Image img = load_image_asset(asset);
  int s = ui_get_scale();
  // 1. Only WHITE counts.
  // 2. Assumes opaque rectangles.

  Color* colors = img.data;
  int w = img.width;
  int h = img.height;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      Color c = colors[y * w + x];
      if (is_bg(c)) {
        continue;
      }
      int x0 = x;
      int y0 = y;
      int x1 = x + 1;
      int y1 = y + 1;
      while (!is_bg(colors[y0 * w + x1])) x1++;
      while (!is_bg(colors[y1 * w + x0])) y1++;
      x1--;
      y1--;
      int ww = x1 - x0 + 1;
      int hh = y1 - y0 + 1;
      // Puts black back
      Rectangle r = {s * x0, s * y0, s * ww, s * hh};
      arrput(out, r);
      for (int xx = x0; xx <= x1; xx++) {
        colors[y0 * w + xx] = BLACK;
        colors[y1 * w + xx] = BLACK;
      }
      for (int yy = y0; yy <= y1; yy++) {
        colors[yy * w + x0] = BLACK;
        colors[yy * w + x1] = BLACK;
      }
    }
  }
  UnloadImage(img);
  return out;
}

// Helper: checks if character is at a word boundary
static bool is_boundary(char prev, char curr) {
  if (prev == '\0') return true;  // Start of string
  if (prev == '/' || prev == '\\' || prev == '_' || prev == '-' || prev == '.')
    return true;
  if (islower(prev) && isupper(curr)) return true;  // camelCase boundary
  return false;
}

// Phase 1: Check if query characters appear in order (case-insensitive)
// Returns true if all query characters found in target in order
bool fuzzy_match_simple(const char* target, const char* query) {
  if (!query || query[0] == '\0') {
    return true;  // Empty query matches everything
  }

  if (!target) {
    return false;
  }

  const char* q = query;
  const char* t = target;

  while (*q != '\0') {
    char qc = tolower(*q);
    bool found = false;

    while (*t != '\0') {
      if (tolower(*t) == qc) {
        found = true;
        t++;
        break;
      }
      t++;
    }

    if (!found) {
      return false;
    }

    q++;
  }

  return true;
}

// Phase 2: Calculate score for matched strings
// Sublime Text style scoring with bonuses for:
// - Consecutive matches
// - Word boundary matches
// - Case-sensitive matches
// - Matches at start of string
int fuzzy_score(const char* target, const char* query) {
  if (!query || query[0] == '\0') {
    return 0;
  }

  int score = 0;
  const char* q = query;
  const char* t = target;
  char prev_t = '\0';
  int consecutive = 0;

  while (*q != '\0' && *t != '\0') {
    char qc = *q;
    char tc = *t;

    // Check for exact case match first
    if (tc == qc) {
      consecutive++;
      score += 100;  // Base score

      // Bonus for consecutive matches
      if (consecutive > 1) {
        score += 50 * consecutive;
      }

      // Bonus for case-sensitive match
      score += 10;

      // Bonus for word boundary match
      if (is_boundary(prev_t, tc)) {
        score += 30;
      }

      q++;
    }
    // Check for case-insensitive match
    else if (tolower(tc) == tolower(qc)) {
      consecutive++;
      score += 100;  // Base score

      // Bonus for consecutive matches
      if (consecutive > 1) {
        score += 50 * consecutive;
      }

      // Bonus for word boundary match
      if (is_boundary(prev_t, tc)) {
        score += 30;
      }

      q++;
    } else {
      consecutive = 0;
    }

    prev_t = tc;
    t++;
  }

  // Bonus for matching at start of string
  if (query[0] != '\0' && target[0] != '\0') {
    char tc = target[0];
    char qc = query[0];
    if (tc == qc || tolower(tc) == tolower(qc)) {
      score += 50;
    }
  }

  return score;
}

// Find all PNG files in a folder
// Returns a dynamic array of file paths (use stb_ds arrfree to free)
// Returns NULL if folder doesn't exist or no PNG files found
char** find_png_files(const char* folder_path) {
  char** png_files = NULL;

  FilePathList files = LoadDirectoryFiles(folder_path);

  for (int i = 0; i < files.count; i++) {
    const char* filepath = files.paths[i];
    int len = strlen(filepath);

    // Check if file ends with .png (case insensitive)
    if (len > 4) {
      const char* ext = &filepath[len - 4];
      if (strcmp(ext, ".png") == 0 || strcmp(ext, ".PNG") == 0) {
        arrput(png_files, clone_string(filepath));
      }
    }
  }

  UnloadDirectoryFiles(files);

  return png_files;
}

// Extract filename without extension from a filepath
// Example: "blueprints/rafa.png" -> "rafa"
// Returns a newly allocated string that must be freed by caller
char* extract_filename_no_ext(const char* filepath) {
  if (!filepath) return NULL;

  // Find the last occurrence of '/' or '\'
  const char* last_slash = strrchr(filepath, '/');
  const char* last_backslash = strrchr(filepath, '\\');
  const char* filename_start = filepath;

  // Use whichever slash appears later
  if (last_slash && last_backslash) {
    filename_start =
        (last_slash > last_backslash) ? last_slash + 1 : last_backslash + 1;
  } else if (last_slash) {
    filename_start = last_slash + 1;
  } else if (last_backslash) {
    filename_start = last_backslash + 1;
  }

  // Find the last '.' for extension
  const char* last_dot = strrchr(filename_start, '.');

  int name_len;
  if (last_dot) {
    name_len = last_dot - filename_start;
  } else {
    name_len = strlen(filename_start);
  }

  // Allocate and copy the filename without extension
  char* result = (char*)malloc(name_len + 1);
  strncpy(result, filename_start, name_len);
  result[name_len] = '\0';

  return result;
}

Vector2 find_modal_off(Rectangle layout) {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  int ww = layout.width;
  int wh = layout.height;
  int s = ui_get_scale();
  int x0 = (sw - ww) / 2;
  int y0 = (sh - wh) / 2;
  x0 = s * (x0 / s);
  y0 = s * (y0 / s);
  Vector2 off = {x0, y0};
  return off;
}

sprite_t* read_text_sprites(const char* desc, const char* root) {
  sprite_t* sprites = NULL;
  const char* nxt = desc;
  while ((nxt = strstr(nxt, "!img:"))) {
    int i = 5;
    while (nxt[i] != ' ' && nxt[i] != '\n') i++;
    char tmp[200];
    strncpy(tmp, nxt + 5, i - 5);
    tmp[i - 5] = '\0';
    char tmp2[300];
    strcpy(tmp2, root);
    strcat(tmp2, tmp);
    // printf("tmp=%s\n", tmp2);
    Texture tex = LoadTexture(tmp2);
    sprite_t sp = {
        .tex = tex,
        .region = (Rectangle){0, 0, tex.width, tex.height},
    };
    arrput(sprites, sp);
    nxt = &nxt[3];
  }
  return sprites;
}

ParsedLayout parse_layout2(const char* fname) {
  ParsedLayout p = {0};
  Image img = LoadImage(fname);
  int s = ui_get_scale();
  // 1. Only WHITE counts.
  // 2. Assumes opaque rectangles.

  Color* colors = img.data;
  int w = img.width;
  int h = img.height;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      Color c = colors[y * w + x];
      if (is_bg(c)) {
        continue;
      }
      int x0 = x;
      int y0 = y;
      int x1 = x + 1;
      int y1 = y + 1;
      while (!is_bg(colors[y0 * w + x1])) x1++;
      while (!is_bg(colors[y1 * w + x0])) y1++;
      Color ci = colors[(y + 1) * w + (x + 1)];
      x1--;
      y1--;
      int ww = x1 - x0 + 1;
      int hh = y1 - y0 + 1;
      // Puts black back
      Rectangle r = {s * x0, s * y0, s * ww, s * hh};
      arrput(p.rects, r);
      arrput(p.colors, ci);
      for (int xx = x0; xx <= x1; xx++) {
        colors[y0 * w + xx] = BLACK;
        colors[y1 * w + xx] = BLACK;
      }
      for (int yy = y0; yy <= y1; yy++) {
        colors[yy * w + x0] = BLACK;
        colors[yy * w + x1] = BLACK;
      }
    }
  }
  UnloadImage(img);
  return p;
}

Rectangle roff(Vector2 off, Rectangle r) {
  return (Rectangle){
      off.x + r.x,
      off.y + r.y,
      r.width,
      r.height,
  };
}

void load_text_sprites(const char* root, const char* txt,
                       sprite_t** out_sprites) {
  const char* nxt = txt;
  sprite_t* sprites = NULL;
  while ((nxt = strstr(nxt, "!img:"))) {
    int i = 5;
    while (nxt[i] != ' ' && nxt[i] != '\n') i++;
    char tmp[200];
    strncpy(tmp, nxt + 5, i - 5);
    tmp[i - 5] = '\0';
    char* p = checkmodpath(root, tmp);
    assert(p);
    printf("Loadin sprite %s ...", p);
    arrput(sprites, load_sprite_raw(p));
    free(p);
    nxt = &nxt[3];
  }
  *out_sprites = sprites;
}

int dofile_with_traceback(lua_State* L, const char* filename) {
  // Push debug.traceback as error handler
  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "traceback");
  lua_remove(L, -2);  // remove debug table
  int error_handler = lua_gettop(L);

  // Load the file (luaL_loadfile compiles but doesn't execute)
  int load_result = luaL_loadfile(L, filename);

  if (load_result != LUA_OK) {
    // Compilation error (syntax error, file not found, etc.)
    const char* err = lua_tostring(L, -1);
    if (err) {
      printf("load error:\n%s\n", err);
      ui_crash(err);
    }
    lua_pop(L, 1);  // pop error
    lua_pop(L, 1);  // pop error handler
    return load_result;
  }

  // Execute the loaded chunk with error handler
  int result = lua_pcall(L, 0, LUA_MULTRET, error_handler);
  if (result != LUA_OK) {
    const char* err = lua_tostring(L, -1);
    if (err) {
      printf("Runtime error:\n%s\n", err);
      ui_crash(err);
    }
    lua_pop(L, 1);  // pop error
  }

  lua_pop(L, 1);  // pop error handler
  return result;
}

const char* randid() {
  int length = 8;
  static char tmp[20];
  const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  // srand() should be called once at program startup, not here
  for (int i = 0; i < length; i++) {
    tmp[i] = charset[rand() % (sizeof(charset) - 1)];
  }
  tmp[length] = '\0';
  printf("ID=%s\n", tmp);
  return tmp;
}

void delete_file(const char* path) {
  // TODO: Check if file is in "safe/allowed" paths
  if (FileExists(path)) {
    int r = remove(path);
    assert(r == 0);
  }
}

const char* get_roman_number(int i) {
  switch (i) {
    case 1:
      return "I";
    case 2:
      return "II";
    case 3:
      return "III";
    case 4:
      return "IV";
    case 5:
      return "V";
    case 6:
      return "VI";
    case 7:
      return "VII";
    case 8:
      return "VIII";
    case 9:
      return "IX";
    case 10:
      return "X";
    case 11:
      return "XI";
    case 12:
      return "XII";
    case 13:
      return "XIII";
    case 14:
      return "XIV";
    case 15:
      return "XV";
    case 16:
      return "XVI";
    case 17:
      return "XVII";
    case 18:
      return "XVIII";
    case 19:
      return "XIX";
    case 20:
      return "XX";
    case 21:
      return "XXI";
    case 22:
      return "XXII";
    case 23:
      return "XXIII";
    case 24:
      return "XXIV";
    case 25:
      return "XXV";
    case 26:
      return "XXVI";
    case 27:
      return "XXVII";
    case 28:
      return "XXVIII";
    case 29:
      return "XXIX";
    case 30:
      return "XXX";
  }
  return "";
}

void draw_bg(Rectangle r) {
  draw_tiled_rect(ui_get_scale(), ui_get_sprites(), rect_bg_pattern, r);
}
