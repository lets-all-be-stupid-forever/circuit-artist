#include "font.h"

#include "assert.h"
#include "colors.h"
#include "img.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

typedef struct {
  // Atlas image.
  Image atlas;
  // offset of the END of each char (starting with "!")
  // the offset column is blank.
  int* offset;
  int line_height;
  // maximum char size (used for empty space)
  int charsize;
  // texture used for drawing directly to textures.
  Texture2D tex;
} ArtFont;

// ASCII Table
// 0  NUL (null)                       32  SPACE     64  @         96  `
// 1  SOH (start of heading)           33  !         65  A         97  a
// 2  STX (start of text)              34  "         66  B         98  b
// 3  ETX (end of text)                35  #         67  C         99  c
// 4  EOT (end of transmission)        36  $         68  D        100  d
// 5  ENQ (enquiry)                    37  %         69  E        101  e
// 6  ACK (acknowledge)                38  &         70  F        102  f
// 7  BEL (bell)                       39  '         71  G        103  g
// 8  BS  (backspace)                  40  (         72  H        104  h
// 9  TAB (horizontal tab)             41  )         73  I        105  i
// 10  LF  (NL line feed, new line)    42  *         74  J        106  j
// 11  VT  (vertical tab)              43  +         75  K        107  k
// 12  FF  (NP form feed, new page)    44  ,         76  L        108  l
// 13  CR  (carriage return)           45  -         77  M        109  m
// 14  SO  (shift out)                 46  .         78  N        110  n
// 15  SI  (shift in)                  47  /         79  O        111  o
// 16  DLE (data link escape)          48  0         80  P        112  p
// 17  DC1 (device control 1)          49  1         81  Q        113  q
// 18  DC2 (device control 2)          50  2         82  R        114  r
// 19  DC3 (device control 3)          51  3         83  S        115  s
// 20  DC4 (device control 4)          52  4         84  T        116  t
// 21  NAK (negative acknowledge)      53  5         85  U        117  u
// 22  SYN (synchronous idle)          54  6         86  V        118  v
// 23  ETB (end of trans. block)       55  7         87  W        119  w
// 24  CAN (cancel)                    56  8         88  X        120  x
// 25  EM  (end of medium)             57  9         89  Y        121  y
// 26  SUB (substitute)                58  :         90  Z        122  z
// 27  ESC (escape)                    59  ;         91  [        123  {
// 28  FS  (file separator)            60  <         92  \        124  |
// 29  GS  (group separator)           61  =         93  ]        125  }
// 30  RS  (record separator)          62  >         94  ^        126  ~
// 31  US  (unit separator)            63  ?         95  _        127  DEL
//
// Chars to be used in the font creation
// CHARS = !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~

static const char first_char = '!';
static const int NUM_CHARS = 94;
static ArtFont _font = {0};

static void ParseFont(ArtFont* f)
{
  int w = f->atlas.width;
  int h = f->atlas.height;
  Color* colors = GetPixels(f->atlas);
  f->line_height = h;
  f->offset[0] = 0;
  int ichar = 1;
  int charsize = 0;
  int start = 0;
  while (true) {
    int xl = start + 1;
    // Finds the next blank column after the start column of a char.
    while (xl < w) {
      bool is_last = true;
      for (int y = 0; y < h; y++) {
        int idx = y * w + xl;
        Color c = colors[y * w + xl];
        if (c.a != 0) {
          // Means this is not a last line, so we should try the next.
          is_last = false;
        }
        // Removes everything that is not white.
        // Indeed, sometimes we want to add "black" pixels in the font image because
        // a char contains blank columns, for example the " char.
        // Then, this additional pass removes that column.
        if (!COLOR_EQ(c, WHITE)) {
          colors[idx] = BLANK;
        }
      }
      if (!is_last) {
        xl = xl + 1;
      }
      else {
        break;
      }
    }
    // Sets the offset to this blank column
    int size = xl - start - 2;
    start = xl + 1;
    f->offset[ichar++] = start;

    // updates the biggest char size
    if (size > charsize) {
      charsize = size;
    }
    // sets the start of the next char as the first column after the blank
    // column. It assumes the chars are spaced by exactly 1 pixel.

    // If the start column is outside the atlas image, we stop.
    if (start >= w) {
      break;
    }
  }
  f->charsize = charsize;
  // Ensure we have the coorect number of chars parsed.
  assert(ichar == NUM_CHARS + 1);
}

static ArtFont CreateFontFromFile(const char* font_filename)
{
  ArtFont fnt = {0};
  fnt.atlas = LoadImage(font_filename);
  fnt.offset = calloc(257, sizeof(int));
  ParseFont(&fnt);
  fnt.tex = LoadTextureFromImage(fnt.atlas);
  return fnt;
}

Vector2 GetRenderedTextSize(const char* txt)
{
  int len = strlen(txt);
  const char last_char = first_char + NUM_CHARS;
  int tot_len = 0;
  for (int i = 0; i < len; i++) {
    int c = txt[i];
    if (c < first_char) c = ' ';
    if (c > last_char) c = ' ';
    if (c == ' ') {
      tot_len += _font.charsize;
      continue;
    }
    int ic = c - first_char;
    tot_len += _font.offset[ic + 1] - _font.offset[ic];
  }
  return (Vector2){
      tot_len, _font.line_height};
}

static Vector2 GetRenderedTextSize2(const char* txt, int len)
{
  const char last_char = first_char + NUM_CHARS;
  // No support for newline for now!
  int tot_len = 0;
  for (int i = 0; i < len; i++) {
    int c = txt[i];
    if (c < first_char) c = ' ';
    if (c > last_char) c = ' ';
    if (c == ' ') {
      tot_len += _font.charsize;
      continue;
    }
    if (c == '$') continue;  // Special char for colors
    if (c == '`') continue;  // Special char for colors
    int ic = c - first_char;
    tot_len += _font.offset[ic + 1] - _font.offset[ic];
  }
  return (Vector2){
      tot_len, _font.line_height};
}

Image RenderText(const char* txt, Color color)
{
  int len = strlen(txt);
  const char last_char = first_char + NUM_CHARS;

  // No support for newline for now!
  int tot_len = 0;
  for (int i = 0; i < len; i++) {
    int c = txt[i];
    if (c < first_char) c = ' ';
    if (c > last_char) c = ' ';
    if (c == ' ') {
      tot_len += _font.charsize;
      continue;
    }
    int ic = c - first_char;
    tot_len += _font.offset[ic + 1] - _font.offset[ic];
  }

  Image out = GenImageColor(tot_len, _font.line_height, BLANK);
  Color* ac = GetPixels(_font.atlas);
  Color* dc = GetPixels(out);
  int xx = 0;
  for (int i = 0; i < len; i++) {
    int c = txt[i];
    if (c < first_char) c = ' ';
    if (c > last_char) c = ' ';
    if (c == ' ') {
      xx += _font.charsize;
      continue;
    }
    int ic = c - first_char;
    int x0 = _font.offset[ic + 0];
    int x1 = _font.offset[ic + 1] - 1;
    int h = _font.line_height;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < x1 - x0; x++) {
        dc[y * out.width + xx + x] = ac[y * _font.atlas.width + x0 + x].r > 0 ? color : BLANK;
      }
    }
    xx += x1 - x0 + 1;
  }
  return out;
}

void FontDraw(Image* dst, const char* txt, int x, int y, Color c)
{
  Image img = RenderText(txt, WHITE);
  Rectangle src_rec = {
      .x = 0,
      .y = 0,
      .width = img.width,
      .height = img.height,
  };
  Rectangle dst_rec = {
      .x = x,
      .y = y,
      .width = src_rec.width,
      .height = src_rec.height,
  };
  ImageDraw(dst, img, src_rec, dst_rec, c);
  UnloadImage(img);
}

void UnloadArtFont()
{
  if (_font.atlas.width > 0) {
    UnloadTexture(_font.tex);
    UnloadImage(_font.atlas);
  }
  free(_font.offset);
  _font = (ArtFont){0};
}

void FontDrawTexture(const char* txt, int x, int y, Color color)
{
  int len = strlen(txt);
  const char last_char = first_char + NUM_CHARS;
  int xx = 0;
  for (int i = 0; i < len; i++) {
    int c = txt[i];
    if (c < first_char) c = ' ';
    if (c > last_char) c = ' ';
    if (c == ' ') {
      xx += _font.charsize;
      continue;
    }
    int ic = c - first_char;
    int x0 = _font.offset[ic + 0];
    int x1 = _font.offset[ic + 1] - 1;
    int h = _font.line_height;

    Rectangle src = {
        .x = x0,
        .y = 0,
        .width = x1 - x0,
        .height = h,
    };
    Vector2 pos = {
        .x = xx + x,
        .y = y,
    };
    DrawTextureRec(_font.tex, src, pos, color);
    xx += x1 - x0 + 1;
  }
}

void LoadArtFont(const char* filepath)
{
  _font = CreateFontFromFile(filepath);
}

int GetFontLineHeight()
{
  return _font.line_height;
}

static void FontDrawTexture2(const char* txt, int len, int x, int y, Color* color, Color normal, Color special)
{
  const char last_char = first_char + NUM_CHARS;
  int xx = 0;
  for (int i = 0; i < len; i++) {
    int c = txt[i];
    if (c < first_char) c = ' ';
    if (c > last_char) c = ' ';
    if (c == ' ') {
      xx += _font.charsize;
      continue;
    }
    if (c == '$') {
      if (COLOR_EQ(*color, normal)) {
        *color = special;
      }
      else {
        *color = normal;
      }
      continue;
    }
    if (c == '`') {
      if (COLOR_EQ(*color, normal)) {
        Color c1 = GetLutColor(COLOR_BG0);
        *color = c1;
      }
      else {
        *color = normal;
      }
      continue;
    }
    int ic = c - first_char;
    int x0 = _font.offset[ic + 0];      // 0
    int x1 = _font.offset[ic + 1] - 1;  // 2 -> 1
    int h = _font.line_height;

    Rectangle src = {
        .x = x0,
        .y = 0,
        .width = x1 - x0,
        .height = h,
    };
    Vector2 pos = {
        .x = xx + x,
        .y = y,
    };
    DrawTextureRec(_font.tex, src, (Vector2){pos.x + 1, pos.y + 1}, BLACK);
    DrawTextureRec(_font.tex, src, (Vector2){pos.x + 0, pos.y + 1}, BLACK);
    DrawTextureRec(_font.tex, src, (Vector2){pos.x + 1, pos.y + 0}, BLACK);
    DrawTextureRec(_font.tex, src, pos, *color);
    xx += x1 - x0 + 1;
  }
}

void DrawTextBox(const char* text, Rectangle rect, Color first_color, int* height)
{
  int lh = GetFontLineHeight() + 2;
  int n = strlen(text);
  // space size
  int sx = 5;
  int x = rect.x;
  int y = rect.y;

  int linew = rect.width;
  // pointer
  int k = 0;
  int offx = 0;
  int offy = 0;
  Color c = first_color;

  while (true) {
    // Part1: finding the next token
    // A token can be:
    // (i) A word
    // (ii) a newline
    int ki = k;
    bool is_new_line = false;
    while (text[ki] == ' ' && text[ki] != '\0') ki++;
    int k0 = ki;
    if (ki == n) break;
    if (text[ki] == '\n') {
      is_new_line = true;
      ki++;
    }
    else {
      while (text[ki] != ' ' && text[ki] != '\n' && text[ki] != '\0') ki++;
    }

    // Renders token
    if (is_new_line) {
      offy += lh + 1;
      offx = 0;
    }
    else {
      int nc = ki - k0;
      int w = GetRenderedTextSize2(text + k0, nc).x;
      if (w + offx > linew) {
        offy += lh + 1;
        offx = 0;
      }
      // renders
      if (height == NULL) {
        FontDrawTexture2(text + k0, nc, x + offx, y + offy, &c, first_color, YELLOW);
      }
      offx = offx + w + sx;
    }
    k = ki;
    if (k >= n) break;
  }
  if (height != NULL) {
    if (offx == 0) offy -= lh;
    *height = offy + lh;
  }
}

typedef enum {
  TKN_TEXT,
  TKN_HLINE,
  TKN_IMG,
} TextTokenType;

typedef struct {
  TextTokenType type;
  int isprite;
} ParsedTokenType;

static bool StartsWithUnsafe(const char* a, const char* b)
{
  int n = strlen(b);
  for (int i = 0; i < n; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

static ParsedTokenType ParseToken(const char* t, int len)
{
  ParsedTokenType r = {0};
  r.type = TKN_TEXT;
  if (len >= 6) {
    if (StartsWithUnsafe(t, "!img:")) {
      char tt[2];
      tt[1] = '\0';
      tt[0] = t[5];
      int n;
      sscanf(&t[5], "%d", &n);
      r.type = TKN_IMG;
      r.isprite = n;
      return r;
    }
  }
  if (len == 3) {
    if (StartsWithUnsafe(t, "!hl")) {
      r.type = TKN_HLINE;
      return r;
    }
  }
  return r;
}

void DrawTextBoxAdvanced(const char* text, Rectangle rect, Color first_color, Sprite* sprites, int* height)
{
  int lh = GetFontLineHeight() + 2;
  int n = strlen(text);
  // space size
  int sx = 5;
  int x = rect.x;
  int y = rect.y;

  int linew = rect.width;
  // pointer
  int k = 0;
  int offx = 0;
  int offy = 0;
  Color c = first_color;

  while (true) {
    // Part1: finding the next token
    // A token can be:
    // (i) A word
    // (ii) a newline
    int ki = k;
    bool is_new_line = false;
    while (text[ki] == ' ' && text[ki] != '\0') ki++;
    int k0 = ki;
    if (ki == n) break;
    if (text[ki] == '\n') {
      is_new_line = true;
      ki++;
    }
    else {
      while (text[ki] != ' ' && text[ki] != '\n' && text[ki] != '\0') ki++;
    }

    // Renders token
    if (is_new_line) {
      offy += lh + 1;
      offx = 0;
    }
    else {
      int nc = ki - k0;
      ParsedTokenType ptkn = ParseToken(text + k0, nc);
      switch (ptkn.type) {
        case TKN_HLINE: {
          if (offx != 0) {
            offy += lh + 1;
            offx = 0;
          }
          if (height == NULL) {
            DrawRectangle(0, offy, linew, 1, BROWN);
          }
          offy -= 5;
          offx = 0;
          break;
        }
        case TKN_IMG: {
          int k = ptkn.isprite;
          int iw = sprites[k].region.width;
          int ih = sprites[k].region.height;
          if (offx != 0) {
            offy += lh + 1;
            offx = 0;
          }

          offx = (linew - iw) / 2;
          if (height == NULL) {
            DrawTextureRec(sprites[k].tex, sprites[k].region, (Vector2){offx, offy}, WHITE);
          }
          offy += ih;
          offx = 0;

          break;
        }
        case TKN_TEXT: {
          int w = GetRenderedTextSize2(text + k0, nc).x;
          if (w + offx > linew) {
            offy += lh + 1;
            offx = 0;
          }
          // renders
          if (height == NULL) {
            FontDrawTexture2(text + k0, nc, x + offx, y + offy, &c, first_color, YELLOW);
          }
          offx = offx + w + sx;
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

void GetDrawTextBoxSize(const char* text, int lw, int* h, int* w)
{
  int lh = GetFontLineHeight() + 2;
  int n = strlen(text);
  // space size
  int sx = 5;

  int linew = lw;
  // pointer
  int k = 0;
  int offx = 0;
  int offy = 0;
  int maxoffx = 0;

  while (true) {
    // Part1: finding the next token
    // A token can be:
    // (i) A word
    // (ii) a newline
    int ki = k;
    bool is_new_line = false;
    while (text[ki] == ' ' && text[ki] != '\0') ki++;
    int k0 = ki;
    if (ki == n) break;
    if (text[ki] == '\n') {
      is_new_line = true;
      ki++;
    }
    else {
      while (text[ki] != ' ' && text[ki] != '\n' && text[ki] != '\0') ki++;
    }

    // Renders token
    if (is_new_line) {
      offy += lh + 1;
      offx = 0;
    }
    else {
      int nc = ki - k0;
      int w = GetRenderedTextSize2(text + k0, nc).x;
      if (w + offx > linew) {
        offy += lh + 1;
        offx = 0;
      }
      // renders
      offx = offx + w + sx;
      maxoffx = offx > maxoffx ? offx : maxoffx;
    }
    k = ki;
    if (k >= n) break;
  }
  if (offx == 0) offy -= lh;
  *h = offy + lh;
  *w = maxoffx;
}

void FontDrawTextureOutlined(const char* txt, int x, int y, Color c, Color bg)
{
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0) continue;
      FontDrawTexture(txt, x + dx, y + dy, bg);
    }
  }

  FontDrawTexture(txt, x, y, c);
}
