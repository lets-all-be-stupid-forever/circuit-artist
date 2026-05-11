#include "uifont.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "paths.h"

#define PIXEL_FONT_SCALE 2
#define PIXEL_FONT_SCALE_BIG 2
#define NUM_GLYPHS 224
#define FIRST_CP 32
#define ATLAS_W 512

// Adjust this to shift all pixel-font glyphs up (-) or down (+) relative to
// the drawing point. 0 = top of bitmap sits at y; negative = shift up.
#define PIXEL_FONT_OFFSET_Y 0

typedef struct {
  Texture2D tex;
  Image img;  // CPU copy; kept only when keep_img=true
  Rectangle recs[NUM_GLYPHS];
  int advance_x[NUM_GLYPHS];
  int offset_x[NUM_GLYPHS];  // bitmap_left
  int offset_y[NUM_GLYPHS];  // rows from line-top to bitmap-top
  int base_size;
} FTFont;

static struct {
  FT_Library ft_lib;
  FTFont font_ui;
} C = {0};

// ---------- atlas builder ----------

typedef struct {
  unsigned char* data;
  int w, h, stride, btop, bleft, advance;
  FT_Pixel_Mode pixel_mode;
} GlyphBm;

static FTFont build_font(const char* path, int size, bool mono, bool keep_img) {
  FTFont f = {0};

  FT_Face face;
  if (FT_New_Face(C.ft_lib, path, 0, &face) != 0) return f;

  if (face->num_fixed_sizes > 0) {
    FT_Select_Size(face, 0);
    f.base_size = face->available_sizes[0].height;
  } else {
    FT_Set_Pixel_Sizes(face, 0, size);
    f.base_size = size;
  }

  FT_Int32 load_flags =
      FT_LOAD_RENDER |
      (mono && face->num_fixed_sizes == 0 ? FT_LOAD_TARGET_MONO : 0);

  // --- Render all glyphs, store temporarily ---
  GlyphBm gbm[NUM_GLYPHS] = {0};
  int max_btop = 0;

  for (int i = 0; i < NUM_GLYPHS; i++) {
    int cp = FIRST_CP + i;
    if (FT_Load_Char(face, cp, load_flags) != 0) continue;
    FT_GlyphSlot g = face->glyph;
    FT_Bitmap* bm = &g->bitmap;

    gbm[i].w = (int)bm->width;
    gbm[i].h = (int)bm->rows;
    gbm[i].stride = bm->pitch < 0 ? -bm->pitch : bm->pitch;
    gbm[i].btop = g->bitmap_top + 2;
    gbm[i].bleft = g->bitmap_left;
    gbm[i].advance = (int)(g->advance.x >> 6);
    gbm[i].pixel_mode = bm->pixel_mode;

    f.advance_x[i] = gbm[i].advance;
    f.offset_x[i] = gbm[i].bleft;

    int sz = gbm[i].stride * gbm[i].h;
    if (sz > 0) {
      gbm[i].data = malloc(sz);
      memcpy(gbm[i].data, bm->buffer, sz);
    }
    if (g->bitmap_top > max_btop) max_btop = g->bitmap_top;
  }

  // --- Compute atlas layout ---
  int cur_x = 0, cur_y = 0, row_h = 0;
  int gx[NUM_GLYPHS], gy[NUM_GLYPHS];

  for (int i = 0; i < NUM_GLYPHS; i++) {
    int bw = gbm[i].w, bh = gbm[i].h;
    if (cur_x + bw + 1 > ATLAS_W) {
      cur_x = 0;
      cur_y += row_h + 1;
      row_h = 0;
    }
    gx[i] = cur_x;
    gy[i] = cur_y;
    cur_x += bw + 1;
    if (bh + 1 > row_h) row_h = bh + 1;
  }
  int atlas_h = cur_y + row_h + 1;
  if (atlas_h <= 0) atlas_h = 1;

  // --- Blit glyphs into RGBA atlas ---
  unsigned char* pixels = calloc((size_t)ATLAS_W * atlas_h * 4, 1);

  for (int i = 0; i < NUM_GLYPHS; i++) {
    GlyphBm* g = &gbm[i];
    f.recs[i] = (Rectangle){gx[i], gy[i], g->w, g->h};
    f.offset_y[i] = max_btop - g->btop;

    if (!g->data) continue;
    for (int row = 0; row < g->h; row++) {
      for (int col = 0; col < g->w; col++) {
        unsigned char val;
        if (g->pixel_mode == FT_PIXEL_MODE_MONO) {
          unsigned char byte = g->data[row * g->stride + col / 8];
          val = (byte >> (7 - (col & 7))) & 1 ? 255 : 0;
        } else {
          val = g->data[row * g->stride + col];
        }
        int idx = ((gy[i] + row) * ATLAS_W + (gx[i] + col)) * 4;
        pixels[idx + 0] = 255;
        pixels[idx + 1] = 255;
        pixels[idx + 2] = 255;
        pixels[idx + 3] = val;
      }
    }
    free(g->data);
    g->data = NULL;
  }

  f.img = (Image){
      .data = pixels,
      .width = ATLAS_W,
      .height = atlas_h,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
  };
  f.tex = LoadTextureFromImage(f.img);
  SetTextureFilter(f.tex, TEXTURE_FILTER_POINT);

  if (!keep_img) {
    free(pixels);
    f.img = (Image){0};
  }

  FT_Done_Face(face);
  return f;
}

static void ft_font_free(FTFont* f) {
  UnloadTexture(f->tex);
  if (f->img.data) free(f->img.data);
  *f = (FTFont){0};
}

// ---------- glyph helpers ----------

static int glyph_advance(FTFont* f, int cp) {
  int i = cp - FIRST_CP;
  if (i < 0 || i >= NUM_GLYPHS) return f->base_size / 2;
  return f->advance_x[i];
}

static int space_width(int scale) {
  return glyph_advance(&C.font_ui, ' ') * scale;
}

static void draw_glyph(FTFont* f, int cp, float x, float y, int scale,
                       Color color) {
  int i = cp - FIRST_CP;
  if (i < 0 || i >= NUM_GLYPHS) return;
  Rectangle src = f->recs[i];
  if (src.width <= 0 || src.height <= 0) return;
  Rectangle dst = {
      x + f->offset_x[i] * scale,
      y + (f->offset_y[i] + PIXEL_FONT_OFFSET_Y) * scale,
      src.width * scale,
      src.height * scale,
  };
  DrawTexturePro(f->tex, src, dst, (Vector2){0, 0}, 0, color);
}

// ---------- public API ----------

void uifont_load() {
  FT_Init_FreeType(&C.ft_lib);
  char* pixel_path =
      get_asset_path("font/ark-pixel-12px-proportional-latin.bdf");
  C.font_ui = build_font(pixel_path, 12, false, true);
  free(pixel_path);
}

void uifont_unload() {
  ft_font_free(&C.font_ui);
  FT_Done_FreeType(C.ft_lib);
  C.ft_lib = NULL;
}

int uifont_line_height() { return C.font_ui.base_size * PIXEL_FONT_SCALE; }
int uifont_line_height_big() {
  return C.font_ui.base_size * PIXEL_FONT_SCALE_BIG;
}

v2 uifont_text_size(const char* txt) {
  int scale = PIXEL_FONT_SCALE;
  int w = 0, i = 0, n = strlen(txt);
  while (i < n) {
    int bytes = 0;
    int cp = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    w += glyph_advance(&C.font_ui, cp) * scale;
  }
  return (v2){w, uifont_line_height()};
}

v2 uifont_text_size_big(const char* txt) {
  int scale = PIXEL_FONT_SCALE_BIG;
  int w = 0, i = 0, n = strlen(txt);
  while (i < n) {
    int bytes = 0;
    int cp = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    w += glyph_advance(&C.font_ui, cp) * scale;
  }
  return (v2){w, uifont_line_height_big()};
}

// --- CPU rendering (used by uifont_render_text / uifont_draw_image) ---

static void blit_glyph(Image* dst, FTFont* f, int cp, int dx, int dy, int scale,
                       Color tint) {
  int i = cp - FIRST_CP;
  if (i < 0 || i >= NUM_GLYPHS || !f->img.data) return;
  Rectangle src = f->recs[i];
  int bw = (int)src.width, bh = (int)src.height;
  if (bw <= 0 || bh <= 0) return;
  int ddx = dx + f->offset_x[i] * scale;
  int ddy = dy + (f->offset_y[i] + PIXEL_FONT_OFFSET_Y) * scale;
  unsigned char* sp = f->img.data;
  unsigned char* dp = dst->data;

  for (int row = 0; row < bh * scale; row++) {
    int sr = row / scale;
    for (int col = 0; col < bw * scale; col++) {
      int sc = col / scale;
      int ax = (int)src.x + sc, ay = (int)src.y + sr;
      unsigned char a = sp[(ay * ATLAS_W + ax) * 4 + 3];
      if (!a) continue;
      int px = ddx + col, py = ddy + row;
      if (px < 0 || px >= dst->width || py < 0 || py >= dst->height) continue;
      unsigned char* d = dp + (py * dst->width + px) * 4;
      d[0] = tint.r;
      d[1] = tint.g;
      d[2] = tint.b;
      d[3] = (unsigned char)((int)a * tint.a / 255);
    }
  }
}

Image uifont_render_text(const char* txt, Color color) {
  v2 sz = uifont_text_size(txt);
  int w = (int)sz.x, h = uifont_line_height();
  if (w <= 0) w = 1;

  Image img = {
      .data = calloc((size_t)w * h * 4, 1),
      .width = w,
      .height = h,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
  };

  int scale = PIXEL_FONT_SCALE;
  int xx = 0, i = 0, n = strlen(txt);
  while (i < n) {
    int bytes = 0;
    int cp = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    if (cp != ' ') blit_glyph(&img, &C.font_ui, cp, xx, 0, scale, color);
    xx += glyph_advance(&C.font_ui, cp) * scale;
  }
  return img;
}

Image uifont_render_text_1x(const char* txt, Color color) {
  int scale = 1;
  int w = 0, i = 0, n = strlen(txt);
  while (i < n) {
    int bytes = 0;
    int cp = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    w += glyph_advance(&C.font_ui, cp) * scale;
  }
  int h = C.font_ui.base_size;
  if (w <= 0) w = 1;

  Image img = {
      .data = calloc((size_t)w * h * 4, 1),
      .width = w,
      .height = h,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
  };

  int xx = 0;
  i = 0;
  while (i < n) {
    int bytes = 0;
    int cp = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    if (cp != ' ') blit_glyph(&img, &C.font_ui, cp, xx, 0, scale, color);
    xx += glyph_advance(&C.font_ui, cp) * scale;
  }
  return img;
}

void uifont_draw_image(Image* dst, const char* txt, int x, int y, Color c) {
  Image img = uifont_render_text(txt, WHITE);
  Rectangle sr = {0, 0, img.width, img.height};
  Rectangle dr = {x, y, img.width, img.height};
  ImageDraw(dst, img, sr, dr, c);
  UnloadImage(img);
}

// --- GPU drawing ---

static void draw_line(FTFont* f, const char* txt, int x, int y, int scale,
                      Color color) {
  int xx = 0, i = 0, n = strlen(txt);
  while (i < n) {
    int bytes = 0;
    int cp = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    if (cp != ' ') draw_glyph(f, cp, x + xx, y, scale, color);
    xx += glyph_advance(f, cp) * scale;
  }
}

void uifont_draw_texture(const char* txt, int x, int y, Color color) {
  draw_line(&C.font_ui, txt, x, y, PIXEL_FONT_SCALE, color);
}

void uifont_draw_texture_big(const char* txt, int x, int y, Color color) {
  draw_line(&C.font_ui, txt, x, y, PIXEL_FONT_SCALE_BIG, color);
}

void uifont_draw_texture_outlined(const char* txt, int x, int y, Color c,
                                  Color bg) {
  for (int dx = -1; dx <= 1; dx++)
    for (int dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0) continue;
      uifont_draw_texture(txt, x + 2 * dx, y + 2 * dy, bg);
    }
  uifont_draw_texture(txt, x, y, c);
}

// --- text-box internals ---

static int word_width(FTFont* f, const char* txt, int len, int scale) {
  int w = 0, i = 0;
  while (i < len) {
    int bytes = 0;
    int c = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    if (c == '$' || c == '`') continue;
    w += glyph_advance(f, c) * scale;
  }
  return w;
}

static void draw_colored_text_shaded(FTFont* f, const char* txt, int len, int x,
                                     int y, int scale, Color* color,
                                     Color normal, Color special) {
  int xx = 0, i = 0;
  while (i < len) {
    int bytes = 0;
    int c = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    if (c == ' ') {
      xx += glyph_advance(f, ' ') * scale;
      continue;
    }
    if (c == '$') {
      *color = COLOR_EQ(*color, normal) ? special : normal;
      continue;
    }
    if (c == '`') {
      *color = COLOR_EQ(*color, normal) ? get_lut_color(COLOR_BG0) : normal;
      continue;
    }
    draw_glyph(f, c, x + xx, y + 2, scale, BLACK);
    draw_glyph(f, c, x + xx, y, scale, *color);
    xx += glyph_advance(f, c) * scale;
  }
}

static void draw_colored_text(FTFont* f, const char* txt, int len, int x, int y,
                              int scale, Color* color, Color normal,
                              Color special) {
  int xx = 0, i = 0;
  while (i < len) {
    int bytes = 0;
    int c = GetCodepointNext(&txt[i], &bytes);
    i += bytes;
    if (c == ' ') {
      xx += glyph_advance(f, ' ') * scale;
      continue;
    }
    if (c == '$') {
      *color = COLOR_EQ(*color, normal) ? special : normal;
      continue;
    }
    if (c == '`') {
      *color = COLOR_EQ(*color, normal) ? get_lut_color(COLOR_BG0) : normal;
      continue;
    }
    draw_glyph(f, c, x + xx, y, scale, *color);
    xx += glyph_advance(f, c) * scale;
  }
}

void uifont_draw_text_box(const char* text, Rectangle rect, Color first_color,
                          int* height) {
  int scale = PIXEL_FONT_SCALE;
  int lh = C.font_ui.base_size * scale + 2;
  int n = strlen(text), sx = space_width(scale);
  int x = rect.x, y = rect.y, linew = rect.width;
  int k = 0, offx = 0, offy = 0;
  Color c = first_color;

  while (true) {
    int ki = k;
    bool is_new_line = false;
    while (text[ki] == ' ' && text[ki] != '\0') ki++;
    int k0 = ki;
    if (ki == n) break;
    if (text[ki] == '\n') {
      is_new_line = true;
      ki++;
    } else
      while (text[ki] != ' ' && text[ki] != '\n' && text[ki] != '\0') ki++;

    if (is_new_line) {
      offy += lh + 1;
      offx = 0;
    } else {
      int nc = ki - k0;
      int w = word_width(&C.font_ui, text + k0, nc, scale);
      if (w + offx > linew) {
        offy += lh + 1;
        offx = 0;
      }
      if (height == NULL)
        draw_colored_text_shaded(&C.font_ui, text + k0, nc, x + offx, y + offy,
                                 scale, &c, first_color, YELLOW);
      offx += w + sx;
    }
    k = ki;
    if (k >= n) break;
  }
  if (height != NULL) {
    if (offx == 0) offy -= lh;
    *height = offy + lh;
  }
}

typedef enum { TKN_TEXT, TKN_HLINE, TKN_IMG } TextTokenType;
typedef struct {
  TextTokenType type;
} ParsedToken;

static bool starts_with(const char* a, const char* b) {
  for (int i = 0; b[i]; i++)
    if (a[i] != b[i]) return false;
  return true;
}

static ParsedToken parse_token(const char* t, int len) {
  if (len >= 6 && starts_with(t, "!img:")) return (ParsedToken){TKN_IMG};
  // if (len == 2 && t[0] == '*' && t[1] == '*') return (ParsedToken){TKN_BOLD};
  if (len == 3 && starts_with(t, "!hl")) return (ParsedToken){TKN_HLINE};
  return (ParsedToken){TKN_TEXT};
}

void uifont_draw_text_box_advanced(const char* text, Rectangle rect,
                                   Color first_color, sprite_t* sprites,
                                   int* height) {
  int scale = PIXEL_FONT_SCALE;
  int lh = C.font_ui.base_size * scale + 2;
  int n = strlen(text), sx = space_width(scale);
  int x = rect.x, y = rect.y, linew = rect.width;
  int k = 0, offx = 0, offy = 0, kcount = 0;
  Color c = first_color;
  FTFont* font = &C.font_ui;
  int cur_scale = scale;

  while (true) {
    int ki = k;
    bool is_new_line = false;
    while (text[ki] == ' ' && text[ki] != '\0') ki++;
    int k0 = ki;
    if (ki == n) break;
    if (text[ki] == '\n') {
      is_new_line = true;
      ki++;
    } else
      while (text[ki] != ' ' && text[ki] != '\n' && text[ki] != '\0') ki++;

    if (is_new_line) {
      offy += lh + 1;
      offx = 0;
    } else {
      int nc = ki - k0;
      ParsedToken ptkn = parse_token(text + k0, nc);
      switch (ptkn.type) {
        case TKN_HLINE:
          if (offx != 0) {
            offy += lh + 1;
            offx = 0;
          }
          if (height == NULL) DrawRectangle(0, offy, linew, 1, BROWN);
          offy -= 5;
          offx = 0;
          break;
        case TKN_IMG: {
          int ki2 = kcount++;
          int iw = sprites[ki2].region.width * 2;
          int ih = sprites[ki2].region.height * 2;
          if (offx != 0) {
            offy += lh + 1;
            offx = 0;
          }
          offx = (linew - iw) / 2;
          if (height == NULL) {
            Rectangle dst = {offx, offy, iw, ih};
            DrawTexturePro(sprites[ki2].tex, sprites[ki2].region, dst,
                           (Vector2){0, 0}, 0, first_color);
          }
          offy += ih;
          offx = 0;
          break;
        }
        case TKN_TEXT: {
          int w = word_width(font, text + k0, nc, cur_scale);
          if (w + offx > linew) {
            offy += lh + 1;
            offx = 0;
          }
          if (height == NULL) {
            Color k = BLACK;
            draw_colored_text(font, text + k0, nc, x + offx, y + offy,
                              cur_scale, &c, first_color, YELLOW);
          }
          offx += w + sx;
          break;
        }
      }
    }
    k = ki;
    if (k >= n) break;
  }
  if (height != NULL) {
    if (offx == 0) offy -= lh;
    *height = offy + lh;
  }
}

void uifont_get_text_box_size(const char* text, int lw, int* h, int* w) {
  int scale = PIXEL_FONT_SCALE;
  int lh = C.font_ui.base_size * scale + 2;
  int n = strlen(text), sx = space_width(scale), linew = lw;
  int k = 0, offx = 0, offy = 0, maxoffx = 0;

  while (true) {
    int ki = k;
    bool is_new_line = false;
    while (text[ki] == ' ' && text[ki] != '\0') ki++;
    int k0 = ki;
    if (ki == n) break;
    if (text[ki] == '\n') {
      is_new_line = true;
      ki++;
    } else
      while (text[ki] != ' ' && text[ki] != '\n' && text[ki] != '\0') ki++;

    if (is_new_line) {
      offy += lh + 1;
      offx = 0;
    } else {
      int nc = ki - k0;
      int ww = word_width(&C.font_ui, text + k0, nc, scale);
      if (ww + offx > linew) {
        offy += lh + 1;
        offx = 0;
      }
      offx += ww + sx;
      if (offx > maxoffx) maxoffx = offx;
    }
    k = ki;
    if (k >= n) break;
  }
  if (offx == 0) offy -= lh;
  *h = offy + lh;
  *w = maxoffx;
}
