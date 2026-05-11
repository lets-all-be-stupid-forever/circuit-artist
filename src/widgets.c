#include "widgets.h"

#include <math.h>
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "colors.h"
#include "sound.h"
#include "ui.h"
#include "uifont.h"
#include "utils.h"
#include "win_main.h"

#define TEXT_BG ((Color){0, 0, 0, 175})

static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

void draw_sepv(Rectangle r) {
  rlPushMatrix();
  rlTranslatef(r.x, r.y, 0);
  rlScalef(2, 2, 1);
  Texture2D sprites = ui_get_sprites();
  NPatchInfo np = {.source = {512, 171, 2, 5}, 2, 0, 0, 0, NPATCH_NINE_PATCH};
  Rectangle dest = {0, 0, r.width / 2, r.height / 2};
  DrawTextureNPatch(sprites, np, dest, (Vector2){0, 0}, 0, WHITE);
  rlPopMatrix();
}

void draw_frame(Rectangle r) {
  rlPushMatrix();
  rlTranslatef(r.x, r.y, 0);
  rlScalef(2, 2, 1);
  Texture2D sprites = ui_get_sprites();
  NPatchInfo np = {.source = {528, 168, 8, 8}, 3, 3, 3, 3, NPATCH_NINE_PATCH};
  Rectangle dest = {-3, -3, r.width / 2 + 6, r.height / 2 + 6};
  DrawTextureNPatch(sprites, np, dest, (Vector2){0, 0}, 0, WHITE);
  rlPopMatrix();
}

static struct {
  Texture ui; /* ui sprites */
} C = {0};

void widgets_init() { C.ui = ui_get_sprites(); }

bool rect_hover(Rectangle hitbox, Vector2 pos) {
  return CheckCollisionPointRec(pos, hitbox);
}

void listbox_init(Listbox* l) {
  *l = (Listbox){0};
  l->row_pad = 8;
  scroll_init(&l->scroll);
}

void listbox_set_box(Listbox* l, Rectangle r) {
  l->hitbox = r;
  scroll_set_content_box(&l->scroll, r, 24);
}

void listbox_add_row_icon(Listbox* l, const char* content, sprite_t sprite) {
  ListboxRow row = {0};
  int w = l->hitbox.width;
  int y = l->height;
  int row_height = uifont_line_height() + 2 * l->row_pad;
  row.sprite = sprite;
  row.text_y = l->row_pad;
  row.content = clone_string(content);
  row.hitbox_r = (Rectangle){
      0,
      y,
      w,
      row_height,
  };
  l->height += row.hitbox_r.height;
  arrput(l->rows, row);
}

void listbox_add_row(Listbox* l, const char* content) {
  listbox_add_row_icon(l, content, (sprite_t){0});
}

static void listbox_update_row_sizes(Listbox* l) {
  l->height = 0;
  for (int i = 0; i < arrlen(l->rows); i++) {
    int w = l->hitbox.width;
    int y = l->height;
    int row_height = uifont_line_height() + 2 * l->row_pad;
    l->rows[i].text_y = l->row_pad;
    l->rows[i].hitbox_r = (Rectangle){0, y, w, row_height};
    l->height += l->rows[i].hitbox_r.height;
  }
}

void listbox_update(Listbox* l) {
  Vector2 mouse = GetMousePosition();
  listbox_update_row_sizes(l);
  int sy = -l->scroll.value;
  l->row_hit = -1;
  Vector2 mouse_ui = {mouse.x, mouse.y};
  scroll_update(&l->scroll, l->height, mouse_ui);
  bool content_hit = CheckCollisionPointRec(mouse_ui, l->hitbox);
  if (content_hit && !l->scroll.hit) {
    ui_inc_hit_count();
    Vector2 rmouse = {
        mouse_ui.x - l->hitbox.x,
        mouse_ui.y - l->hitbox.y - sy,
    };
    for (int i = 0; i < arrlen(l->rows); i++) {
      Rectangle hitbox = l->rows[i].hitbox_r;
      bool hit = CheckCollisionPointRec(rmouse, hitbox);
      l->rows[i].hitbox_g = (Rectangle){
          hitbox.x + l->hitbox.x,
          hitbox.y + l->hitbox.y - sy,
          hitbox.width,
          hitbox.height,
      };
      if (hit) {
        l->row_hit = i;
        break;
      }
    }
  }
}

static void draw_row(int row_w, int row_h, ListboxRow* row, bool sel) {
  Color bg = BLANK;
  Color fg = CA_WHITE;
  if (sel) {
    bg = get_lut_color(COLOR_ORANGE);
    fg = BLACK;
  }
  DrawRectangle(0, 0, row_w, row_h, bg);
  rlPushMatrix();
  if (row->sprite.region.width > 0) {
    int tw = row->sprite.region.width * 2;
    int th = row->sprite.region.height * 2;
    int y = (row_h - th) / 2;
    Rectangle dst = {2, y, tw, th};
    DrawTexturePro(row->sprite.tex, row->sprite.region, dst, (Vector2){0, 0}, 0,
                   fg);
    rlTranslatef(2 + 4 + tw, 0, 0);
  }
  int pad = 2;
  uifont_draw_texture(row->content, pad, row->text_y, fg);
  rlPopMatrix();
}

void listbox_draw(Listbox* l, int selected) {
  Rectangle box = l->hitbox;
  BeginScissorMode(box.x, box.y, box.width, box.height);
  rlPushMatrix();
  rlTranslatef(box.x, box.y, 0);
  DrawRectangle(0, 0, box.width, box.height, TEXT_BG);
  rlTranslatef(0, -l->scroll.value, 0);
  for (int i = 0; i < arrlen(l->rows); i++) {
    ListboxRow* row = &l->rows[i];
    draw_row(row->hitbox_r.width, row->hitbox_r.height, row, selected == i);
    rlTranslatef(0, row->hitbox_r.height, 0);
  }
  rlPopMatrix();
  scroll_draw(&l->scroll);
  EndScissorMode();
  draw_frame(box);
}

// scroll I need: 1. value, some state for hovering: mouse0, value0
// value, mouse0, value0
void listbox_clear(Listbox* l) {
  for (int i = 0; i < arrlen(l->rows); i++) {
    free(l->rows[i].content);
  }
  arrfree(l->rows);
  l->rows = NULL;
  l->scroll.value = 0;
  //  l->scroll = (Scroll){0};
}

void listbox_destroy(Listbox* l) {
  for (int i = 0; i < arrlen(l->rows); i++) {
    free(l->rows[i].content);
  }
  arrfree(l->rows);
  *l = (Listbox){0};
}

void scroll_init(Scroll* s) { *s = (Scroll){0}; }

void scroll_set_content_box(Scroll* s, Rectangle box, int w) {
  s->content = box;
  s->scroll_rect = (Rectangle){box.x + box.width - w, box.y, w, box.height};
}

void scroll_reset_value(Scroll* s) { s->value = 0; }

int scroll_get_value(Scroll* s) { return s->value; }

static int fclip(int s, int vmin, int vmax) {
  if (s < vmin) return vmin;
  if (s > vmax) return vmax;
  return s;
}

static int scroll_get_max_value(Scroll* s, float h) {
  int th = s->scroll_rect.height;
  int max_value = fclip(h - th, 0, 100000);
  return max_value;
}

void scroll_scroll_to_bottom(Scroll* s, float h) {
  int max_value = scroll_get_max_value(s, h);
  s->value = max_value;
  int th = s->scroll_rect.height;
  int srect = (th * th) / h;
  int y = (s->value * th) / h;
  if (srect < th) {
    s->rect.x = s->scroll_rect.x;
    s->rect.y = s->scroll_rect.y + y;
    s->rect.width = s->scroll_rect.width;
    s->rect.height = srect;
  } else {
    s->hidden = true;
    s->rect.width = 0;
    s->rect.height = 0;
  }
}

void scroll_update(Scroll* s, float h, Vector2 mouse) {
  int th = s->scroll_rect.height;
  int max_value = scroll_get_max_value(s, h);
  s->hit = false;
  if (h < s->content.height) {
    s->hidden = true;
    s->value = 0;
    return;
  } else {
    s->hidden = false;
  }

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && s->down) {
    s->down = false;
  }

  if (s->down) {
    int dy = s->mouse0 - mouse.y;
    s->value = s->value0 - (dy * h) / th;
  }

  if (CheckCollisionPointRec(mouse, s->content)) {
    float dy = GetMouseWheelMove();
    if (fabs(dy) > 0.1) {
      s->value = s->value - 30 * dy;
    }
  }

  s->hit = CheckCollisionPointRec(mouse, s->scroll_rect);
  if (s->hit) {
    ui_inc_hit_count();
    if (CheckCollisionPointRec(mouse, s->rect)) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        s->down = true;
        s->mouse0 = mouse.y;
        s->value0 = s->value;
      }
    }
  }
  s->value = fclip(s->value, 0, max_value);

  int srect = (th * th) / h;
  int y = (s->value * th) / h;
  if (srect < th) {
    s->rect.x = s->scroll_rect.x;
    s->rect.y = s->scroll_rect.y + y;
    s->rect.width = s->scroll_rect.width;
    s->rect.height = srect;
  } else {
    s->hidden = true;
    s->rect.width = 0;
    s->rect.height = 0;
  }
}

void scroll_draw(Scroll* s) {
  rlPushMatrix();

  if (!s->hidden) {
    Color bg_color = get_lut_color(COLOR_BTN0);
    DrawRectangleRec(s->scroll_rect, bg_color);
    Color c1 = get_lut_color(COLOR_BTN1);
    if (s->down) {
      c1 = get_lut_color(COLOR_ORANGE);
    }
    Btn b = {0};
    b.hitbox = s->rect;
    // b.hitbox.width -= 2;
    b.pressed = s->down;
    btn_draw_text(&b, "");
  }
  rlPopMatrix();
}

void textbox_set_bg(Textbox* t, Color bg) { t->bg = bg; }

void textbox_init(Textbox* t) {
  *t = (Textbox){0};
  // t->bg = BLACK;
  t->bg = TEXT_BG;
  scroll_init(&t->scroll);
}

void textbox_calc_height(Textbox* t) {
  /* Computes text height and width */
  if (!t->text) return;
  t->text_x = 12;
  t->text_w = t->box.width - t->scroll.scroll_rect.width - 2 * t->text_x;
  int th;
  uifont_draw_text_box_advanced(t->text, (Rectangle){0, 0, t->text_w, 0},
                                CA_WHITE, t->sprites, &th);
  t->height = th;

  scroll_reset_value(&t->scroll);
}

void textbox_set_content(Textbox* t, const char* txt, sprite_t* sprites) {
  if (t->text) {
    free(t->text);
  }
  t->text = clone_string(txt);
  t->sprites = sprites;
  textbox_calc_height(t);
}

void textbox_update(Textbox* t) {
  Vector2 mouse = GetMousePosition();
  scroll_update(&t->scroll, t->height, mouse);
  if (CheckCollisionPointRec(mouse, t->box)) {
    ui_inc_hit_count();
  }
}

void textbox_draw(Textbox* t) {
  Rectangle box = t->box;
  BeginScissorMode(box.x, box.y, box.width, box.height);
  rlPushMatrix();
  rlTranslatef(box.x, box.y, 0);
  DrawRectangle(0, 0, box.width, box.height, t->bg);
  rlTranslatef(t->text_x, -t->scroll.value, 0);
  uifont_draw_text_box_advanced(t->text, (Rectangle){0, 0, t->text_w, 0},
                                CA_WHITE, t->sprites, NULL);
  rlPopMatrix();
  EndScissorMode();
  scroll_draw(&t->scroll);
  draw_frame(t->box);
}

void textbox_destroy(Textbox* t) {
  if (t->text) free(t->text);
}

void textbox_set_box(Textbox* t, Rectangle box) {
  bool changed = (t->box.x != box.x || t->box.y != box.y ||
                  t->box.width != box.width || t->box.height != box.height);
  t->box = box;
  scroll_set_content_box(&t->scroll, box, 24);
  if (changed) textbox_calc_height(t);
}

// Updates button state. Returns true if button was clicked.
bool btn_update(Btn* b) {
  b->hover = false;
  Vector2 pos = GetMousePosition();
  bool hit = CheckCollisionPointRec(pos, b->hitbox) &&
             ui_get_hit_count() == 0 && !b->hidden;
  if (hit) {
    ui_inc_hit_count();
  }
  b->hover = hit;
  if (b->disabled || b->hidden) return false;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hit) {
    b->pressed = true;
  }

  bool ret = false;
  // handling Click callback
  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    if (hit && b->pressed) {
      ret = true;
    }
    b->pressed = false;
  }
  if (ret) {
    play_sound_click();
  }
  return ret;
}

static void draw_btn_base_done(Btn* b) {
  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  rlPushMatrix();
  rlTranslatef(x, y, 0);
  rlScalef(2, 2, 1);
  Texture2D sprites = ui_get_sprites();
  NPatchInfo np = {.source = {528, 139, 5, 5}, 2, 2, 2, 2, NPATCH_NINE_PATCH};
  if (b->pressed || b->toggled) {
    np =
        (NPatchInfo){.source = {534, 139, 5, 5}, 2, 2, 2, 2, NPATCH_NINE_PATCH};
  }
  Rectangle dest = {0, 0, w / 2, h / 2};
  DrawTextureNPatch(sprites, np, dest, (Vector2){0, 0}, 0, WHITE);
  rlPopMatrix();
}

static void draw_btn_base_primary(Btn* b) {
  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  rlPushMatrix();
  rlTranslatef(x, y, 0);
  rlScalef(2, 2, 1);
  Texture2D sprites = ui_get_sprites();
  NPatchInfo np = {.source = {528, 123, 5, 5}, 2, 2, 2, 2, NPATCH_NINE_PATCH};
  if (b->pressed || b->toggled) {
    np =
        (NPatchInfo){.source = {534, 123, 5, 5}, 2, 2, 2, 2, NPATCH_NINE_PATCH};
  }
  Rectangle dest = {0, 0, w / 2, h / 2};
  DrawTextureNPatch(sprites, np, dest, (Vector2){0, 0}, 0, WHITE);
  rlPopMatrix();
}

static void draw_btn_base(Btn* b) {
  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  rlPushMatrix();
  rlTranslatef(x, y, 0);
  rlScalef(2, 2, 1);
  Texture2D sprites = ui_get_sprites();
  NPatchInfo np = {.source = {528, 112, 5, 5}, 2, 2, 2, 2, NPATCH_NINE_PATCH};
  if ((b->pressed || b->toggled) && !b->disabled) {
    np =
        (NPatchInfo){.source = {534, 112, 5, 5}, 2, 2, 2, 2, NPATCH_NINE_PATCH};
  }
  Rectangle dest = {0, 0, w / 2, h / 2};
  DrawTextureNPatch(sprites, np, dest, (Vector2){0, 0}, 0, WHITE);
  rlPopMatrix();
}

void btn_draw_text(Btn* b, const char* text) {
  if (b->hidden) return;
  Vector2 size = uifont_text_size(text);
  int fx = (b->hitbox.x + b->hitbox.width / 2) - size.x / 2;
  int fy = (b->hitbox.y + b->hitbox.height / 2) - size.y / 2;
  if (b->pressed) {
    // fy += 2;
  }

  Color c0 = get_lut_color(COLOR_BTN0);
  Color c2 = get_lut_color(COLOR_BTN2);

  if (b->primary) {
    draw_btn_base_primary(b);
  } else if (b->done) {
    draw_btn_base_done(b);
  } else {
    draw_btn_base(b);
  }

  if (!b->disabled) {
    if (!b->gradient) {
      uifont_draw_texture(text, fx, fy, BLACK);
    } else {
      uifont_draw_texture(text, fx, fy + 2, BLACK);
      uifont_draw_texture(text, fx, fy, CA_WHITE);
    }
  } else {
    if (!b->primary) {
      uifont_draw_texture(text, fx + 1, fy, c2);
      uifont_draw_texture(text, fx, fy + 1, c2);
      uifont_draw_texture(text, fx + 1, fy + 1, c2);
      uifont_draw_texture(text, fx, fy, c0);
    } else {
      Color bg = (Color){253, 217, 0, 255};
      Color fg = (Color){128, 38, 1, 255};
      uifont_draw_texture(text, fx + 1, fy, bg);
      uifont_draw_texture(text, fx, fy + 1, bg);
      uifont_draw_texture(text, fx + 1, fy + 1, bg);
      uifont_draw_texture(text, fx, fy, fg);
    }
  }
}

void btn_draw_icon2(Btn* b, int ui_scale, Texture2D texture, Rectangle source) {
  if (b->hidden) return;
  int s = 2;
  int fx = (b->hitbox.x + b->hitbox.width / 2) - s * source.width / 2;
  int fy = (b->hitbox.y + b->hitbox.height / 2) - s * source.height / 2;
  // if (b->pressed) fy += 1;
  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  Color c0 = get_lut_color(COLOR_BTN0);
  Color c1 = get_lut_color(COLOR_BTN1);
  Color c2 = get_lut_color(COLOR_BTN2);
  Color cbg = get_lut_color(COLOR_BTN_BG);
  if (b->primary) {
    draw_btn_base_primary(b);
  } else if (b->done) {
    draw_btn_base_done(b);
  } else {
    draw_btn_base(b);
  }
  rlPushMatrix();
  rlTranslatef(fx, fy, 0);
  rlScalef(s, s, 1);
  if (!b->disabled) {
    if (!b->gradient) {
      DrawTextureRec(texture, source, (Vector2){0, 0}, BLACK);
    } else {
      DrawTextureRec(texture, source, (Vector2){0, 0 + 1}, BLACK);
      DrawTextureRec(texture, source, (Vector2){0, 0}, WHITE);
    }
  } else {
    if (!b->primary) {
      DrawTextureRec(texture, source, (Vector2){0 + 1, 0}, c2);
      DrawTextureRec(texture, source, (Vector2){0, 0 + 1}, c2);
      DrawTextureRec(texture, source, (Vector2){0 + 1, 0 + 1}, c2);
      DrawTextureRec(texture, source, (Vector2){0, 0}, c0);
    } else {
      Color bg = (Color){253, 217, 0, 255};
      Color fg = (Color){128, 38, 1, 255};
      DrawTextureRec(texture, source, (Vector2){0 + 1, 0}, bg);
      DrawTextureRec(texture, source, (Vector2){0, 0 + 1}, bg);
      DrawTextureRec(texture, source, (Vector2){0 + 1, 0 + 1}, bg);
      DrawTextureRec(texture, source, (Vector2){0, 0}, fg);
    }
  }
  rlPopMatrix();
}

// Icon button: tool buttons and challenge button.
void btn_draw_icon(Btn* b, Rectangle source) {
  btn_draw_icon2(b, 2, C.ui, source);
}

// Draws the legend of the button. Needs to be called separately from the
// button itself.
void btn_draw_legend(Btn* b, const char* text) {
  if (b->hidden) return;
  Vector2 pos = GetMousePosition();
  bool hover = b->hover;
  if (hover) {
    int x = pos.x + 16;
    int y = pos.y + 16;
    Color fc = CA_WHITE;
    int s = 2;
    int bh, bw;
    int lw = 900;
    uifont_get_text_box_size(text, lw, &bh, &bw);
    Rectangle area = {x, y, bw + 6, bh + 6};
    if (y > GetScreenHeight() / 2) {
      area.y = area.y - area.height - 16 - 8;
      y = area.y;
    }
    if (x > GetScreenWidth() / 2) {
      area.x = area.x - area.width - 16;
      x = area.x;
    }
    DrawRectangle(area.x - s, area.y - s, area.width + 2 * s,
                  area.height + 2 * s, BLACK);
    draw_bg(area);
    DrawRectangleRec(area, TEXT_BG);
    rlPushMatrix();
    y = y + 4;
    x = x + 4;
    uifont_draw_text_box(text, (Rectangle){x, y, lw, 0}, fc, NULL);
    rlPopMatrix();
  }
}

// Checks if button is being hovered
bool btn_hover(Btn* b) {
  if (b->disabled || b->hidden) return false;
  Vector2 pos = GetMousePosition();
  bool hit = CheckCollisionPointRec(pos, b->hitbox);
  return hit;
}

// Colorbox button (displayed in the bottom of the UI.
void btn_draw_color_v(Rectangle r, Color c, bool selected, bool disabled) {
  int s = ui_get_scale();
  int p = 2;
  Rectangle r0 = (Rectangle){
      r.x - p,
      r.y - p,
      r.width + 2 * p,
      r.height + 2 * p,
  };
  Rectangle r2 = (Rectangle){
      r.x + p,
      r.y + p,
      r.width - 2 * p,
      r.height - 2 * p,
  };
  if (selected) {
    DrawRectangleRec(r0, YELLOW);
  } else {
    DrawRectangleRec(r0, BLACK);
  }
  DrawRectangleRec(r2, c);

#if 0
#define rect_sel ((Rectangle){688, 0, 17, 17})
  if (selected) {
    Texture2D sprites = ui_get_sprites();
    rlPushMatrix();
    rlTranslatef(r.x, r.y, 0);
    rlScalef(2, 2, 1);
    Rectangle dst = {0, 0, 17, 17};
    DrawTexturePro(sprites, rect_sel, dst, (Vector2){0}, 0, BLACK);
    rlPopMatrix();
  }
#endif
}

void btn_draw_color(Rectangle r, Color c, bool selected, bool disabled) {
  int s = ui_get_scale();
  int x = r.x + 1 * s;
  int y = r.y + 1 * s;
  int w = r.width - 2 * s;
  int h = r.height - 2 * s;
  Color c0 = get_lut_color(COLOR_BTN0);
  Color c1 = get_lut_color(COLOR_BTN1);
  Color c2 = get_lut_color(COLOR_BTN2);
  Color cbg = get_lut_color(COLOR_BTN_BG);
  if (!disabled && selected) {
    DrawRectangle(x - s, y - s, w + 2 * s, h + 2 * s, YELLOW);
  }
  DrawRectangle(x, y, w, h, c1);
  DrawRectangle(x, y, w - s, s, c0);
  DrawRectangle(x, y, s, h - s, c0);
  DrawRectangle(x, y + h - s, w, s, c2);
  DrawRectangle(x + w - s, y, s, h, c2);
  DrawRectangle(x + s, y + s, w - 3 * s, s, BLACK);
  DrawRectangle(x + s, y + s, s, h - 3 * s, BLACK);
  if (disabled) {
    DrawRectangle(x + 2 * s, y + 2 * s, w - 4 * s, h - 4 * s, c1);
  } else {
    DrawRectangle(x + 2 * s, y + 2 * s, w - 4 * s, h - 4 * s, c);
  }
}

// Draws a basic frame of size 2px.
void draw_widget_frame(Rectangle r) {
  int s = ui_get_scale();
  int x = r.x - 2 * s;
  int y = r.y - 2 * s;
  int w = r.width + 4 * s;
  int h = r.height + 4 * s;
  Color c0 = get_lut_color(COLOR_BTN0);
  Color c1 = get_lut_color(COLOR_BTN1);
  Color c2 = get_lut_color(COLOR_BTN2);
  Color cbg = get_lut_color(COLOR_BTN_BG);
  // DrawRectangle(x, y, w, h, c1);
  DrawRectangle(x, y, w - s, s, c0);
  DrawRectangle(x, y, s, h - s, c0);
  DrawRectangle(x, y + h - s, w - s, s, c2);
  DrawRectangle(x + w - s, y, s, h, c2);
  DrawRectangle(x + s, y + h - 2 * s, w - 2 * s, s, c1);
  DrawRectangle(x + w - 2 * s, y + s, s, h - 2 * s, c1);
  DrawRectangle(x + s, y + s, w - 3 * s, s, BLACK);
  DrawRectangle(x + s, y + s, s, h - 3 * s, BLACK);
}

void slider_draw(Slider* slider) {
  int s = ui_get_scale();
  DrawRectangleRec(slider->hitbox, GRAY);

  int hx = slider->hitbox.x;
  int hy = slider->hitbox.y;
  int hw = slider->hitbox.width;
  int hh = slider->hitbox.height;

  int pw = (slider->value * hw) / (slider->maxValue - slider->minValue);
  DrawRectangleRec(slider->hitbox, GRAY);
  DrawRectangle(hx, hy, pw, hh, GREEN);
  DrawRectangle(hx + pw, hy, 10, hh, BLUE);
}

bool slider_update(Slider* slider) {
  Vector2 mouse = GetMousePosition();
  bool hit = CheckCollisionPointRec(mouse, slider->hitbox);
  int hw = slider->hitbox.width;
  int hx = slider->hitbox.x;
  slider->released = false;
  int cv = ((mouse.x - hx) * (slider->maxValue - slider->minValue)) / hw;
  cv = max_int(cv, slider->minValue);
  cv = min_int(cv, slider->maxValue);
  slider->cursorValue = cv;
  if (hit && ui_get_hit_count() == 0) {
    ui_inc_hit_count();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      slider->pressed = true;
    }
  }
  if (slider->pressed) {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      slider->pressed = false;
      slider->released = true;
    }
  }
  return slider->pressed || slider->released;
}

void editbox_init(Editbox* b) {
  b->alive = 0;
  b->active = true;
  b->tlen = 0;
  b->cursor_pos = 0;
  b->txt[0] = '\0';
}

/* true if text has changed */
bool editbox_update(Editbox* b) {
  float dt = GetFrameTime();
  b->alive += dt;
  int key = GetCharPressed();
  int max_input_chars = 255;
  bool ret = false;

  // Handle character input
  while (key > 0) {
    if ((key >= 32) && (key <= 125) && (b->tlen < max_input_chars)) {
      // Insert character at cursor position
      for (int i = b->tlen; i > b->cursor_pos; i--) {
        b->txt[i] = b->txt[i - 1];
      }
      b->txt[b->cursor_pos] = (char)key;
      b->cursor_pos++;
      b->tlen++;
      b->txt[b->tlen] = '\0';
    }
    key = GetCharPressed();  // Check next character in the queue
    b->alive = 0;
    ret = true;
  }

  // Handle backspace
  if (IsKeyPressed(KEY_BACKSPACE) && b->cursor_pos > 0) {
    // Remove character before cursor
    for (int i = b->cursor_pos - 1; i < b->tlen; i++) {
      b->txt[i] = b->txt[i + 1];
    }
    b->cursor_pos--;
    b->tlen--;
    b->alive = 0;
    ret = true;
  }

  // Handle delete key
  if (IsKeyPressed(KEY_DELETE) && b->cursor_pos < b->tlen) {
    // Remove character at cursor
    for (int i = b->cursor_pos; i < b->tlen; i++) {
      b->txt[i] = b->txt[i + 1];
    }
    b->tlen--;
    b->alive = 0;
  }

  // Handle arrow keys
  if (IsKeyPressed(KEY_LEFT) && b->cursor_pos > 0) {
    b->cursor_pos--;
    b->alive = 0;
  }
  if (IsKeyPressed(KEY_RIGHT) && b->cursor_pos < b->tlen) {
    b->cursor_pos++;
    b->alive = 0;
  }
  if (IsKeyPressed(KEY_HOME)) {
    b->cursor_pos = 0;
    b->alive = 0;
  }
  if (IsKeyPressed(KEY_END)) {
    b->cursor_pos = b->tlen;
    b->alive = 0;
  }

  return ret;
}

void editbox_draw(Editbox* b) {
  rlPushMatrix();
  int lh = uifont_line_height();
  int xx = 4;
  int yy = (b->hitbox.height - lh) / 2;
  rlTranslatef(b->hitbox.x + xx, b->hitbox.y + yy, 0);

  // Draw text before cursor (to measure cursor position)
  char temp[256];
  strncpy(temp, b->txt, b->cursor_pos);
  temp[b->cursor_pos] = '\0';
  int cursor_x = uifont_text_size(temp).x;

  // Draw full text
  uifont_draw_texture(b->txt, 1, 1, BLACK);
  uifont_draw_texture(b->txt, 0, 0, CA_WHITE);

  // Draw cursor at cursor_pos
  int cursor_type = ((int)(3 * b->alive)) % 2;
  if (cursor_type == 0) {
    rlTranslatef(cursor_x + 1, 0, 0);
    DrawRectangle(0, 0, 2, lh + 1, BLACK);
    DrawRectangle(0, 0, 1, lh, CA_WHITE);
  }
  rlPopMatrix();
}

void textbox_scroll_to_bottom(Textbox* t) {
  scroll_scroll_to_bottom(&t->scroll, t->height);
}

void label_set_text(Label* l, const char* txt) {
  snprintf(l->txt, sizeof(l->txt), "%s", txt);
}

void custom_label_draw_centered(Rectangle hitbox, const char* txt, Color c,
                                Color cshadow) {
  rlPushMatrix();
  rlTranslatef(hitbox.x, hitbox.y, 0);
  int th = hitbox.height;
  int tw = hitbox.width;
  int lh = uifont_line_height();
  int offy = (th - lh) / 2;
  int wx = uifont_text_size(txt).x;
  int offx = (tw - wx) / 2;
  uifont_draw_texture(txt, offx + 1, offy + 1, cshadow);
  uifont_draw_texture(txt, offx, offy, c);
  rlPopMatrix();
}

void label_draw_centered(Label* l) {
  rlPushMatrix();
  rlTranslatef(l->hitbox.x, l->hitbox.y, 0);
  int th = l->hitbox.height;
  int tw = l->hitbox.width;
  int lh = uifont_line_height();
  int offy = (th - lh) / 2;
  int wx = uifont_text_size(l->txt).x;
  int offx = (tw - wx) / 2;
  uifont_draw_texture(l->txt, offx + 1, offy + 1, BLACK);
  uifont_draw_texture(l->txt, offx, offy, CA_WHITE);
  rlPopMatrix();
}

void label_draw(Label* l) {
  rlPushMatrix();
  rlTranslatef(l->hitbox.x, l->hitbox.y, 0);
  int th = l->hitbox.height;
  int lh = uifont_line_height();
  int offy = (th - lh) / 2;
  uifont_draw_texture_outlined(l->txt, 0, offy, CA_WHITE, BLACK);
  rlPopMatrix();
}

// ---- LineEdit ----

static int le_x_of(LineEdit* e, int pos) {
  if (pos <= 0) return 0;
  static char tmp[LINEEDIT_BUFSIZE];
  memcpy(tmp, e->buf, pos);
  tmp[pos] = '\0';
  return (int)uifont_text_size(tmp).x;
}

static int le_pos_from_x(LineEdit* e, int target_x) {
  int best = 0;
  int best_dist = abs(target_x);
  static char tmp[LINEEDIT_BUFSIZE];
  for (int pos = 1; pos <= e->len; pos++) {
    memcpy(tmp, e->buf, pos);
    tmp[pos] = '\0';
    int x = (int)uifont_text_size(tmp).x;
    int dist = abs(x - target_x);
    if (dist <= best_dist) {
      best = pos;
      best_dist = dist;
    } else {
      break;
    }
  }
  return best;
}

static void le_delete_selection(LineEdit* e) {
  if (e->sel_anchor < 0) return;
  int s_min = min_int(e->cursor, e->sel_anchor);
  int s_max = max_int(e->cursor, e->sel_anchor);
  memmove(e->buf + s_min, e->buf + s_max, e->len - s_max + 1);
  e->len -= (s_max - s_min);
  e->cursor = s_min;
  e->sel_anchor = -1;
  e->alive = 0;
}

static void le_scroll_to_cursor(LineEdit* e) {
  int pad = 8;
  int cx = le_x_of(e, e->cursor);
  int visible_w = (int)e->hitbox.width - 2 * pad;
  if (visible_w < 0) visible_w = 0;
  if (cx < e->scroll_x) e->scroll_x = cx;
  if (cx > e->scroll_x + visible_w) e->scroll_x = cx - visible_w;
  if (e->scroll_x < 0) e->scroll_x = 0;
}

void lineedit_init(LineEdit* e) {
  *e = (LineEdit){0};
  e->sel_anchor = -1;
}

void lineedit_set_text(LineEdit* e, const char* txt) {
  if (!txt) txt = "";
  int n = (int)strlen(txt);
  if (n >= LINEEDIT_BUFSIZE) n = LINEEDIT_BUFSIZE - 1;
  memcpy(e->buf, txt, n);
  e->buf[n] = '\0';
  e->len = n;
  e->cursor = n;
  e->sel_anchor = -1;
  e->alive = 0;
  e->scroll_x = 0;
}

const char* lineedit_get_text(LineEdit* e) { return e->buf; }

void lineedit_set_focus(LineEdit* e, bool focused) {
  e->focused = focused;
  if (!focused) e->sel_anchor = -1;
}

bool lineedit_update(LineEdit* e) {
  float dt = (float)ui_get_frame_time();
  e->alive += dt;

  Vector2 mouse = GetMousePosition();
  int pad = 8;
  bool hit =
      CheckCollisionPointRec(mouse, e->hitbox) && ui_get_hit_count() == 0;

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) e->dragging = false;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hit) {
    lineedit_set_focus(e, false);
  }

  if (hit) {
    ui_inc_hit_count();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      lineedit_set_focus(e, true);
      int mx = (int)(mouse.x - e->hitbox.x - pad + e->scroll_x);
      int clicked = le_pos_from_x(e, mx);
      bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
      if (shift) {
        if (e->sel_anchor < 0) e->sel_anchor = e->cursor;
        e->cursor = clicked;
      } else {
        e->cursor = clicked;
        e->sel_anchor = -1;
        e->dragging = true;
      }
      e->alive = 0;
    }
  }

  if (e->dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    int mx = (int)(mouse.x - e->hitbox.x - pad + e->scroll_x);
    int hovered = le_pos_from_x(e, mx);
    if (hovered != e->cursor) {
      if (e->sel_anchor < 0) e->sel_anchor = e->cursor;
      e->cursor = hovered;
      e->alive = 0;
    }
  }

  if (!e->focused) {
    le_scroll_to_cursor(e);
    return false;
  }

  bool ret = false;
  bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  bool ctrl = is_control_down();

  if (ctrl && IsKeyPressed(KEY_A)) {
    e->sel_anchor = 0;
    e->cursor = e->len;
    e->alive = 0;
  }

  if (ctrl && IsKeyPressed(KEY_C) && e->sel_anchor >= 0) {
    int s_min = min_int(e->cursor, e->sel_anchor);
    int s_max = max_int(e->cursor, e->sel_anchor);
    static char tmp[LINEEDIT_BUFSIZE];
    int n = s_max - s_min;
    memcpy(tmp, e->buf + s_min, n);
    tmp[n] = '\0';
    SetClipboardText(tmp);
  }

  if (ctrl && IsKeyPressed(KEY_X) && e->sel_anchor >= 0) {
    int s_min = min_int(e->cursor, e->sel_anchor);
    int s_max = max_int(e->cursor, e->sel_anchor);
    static char tmp[LINEEDIT_BUFSIZE];
    int n = s_max - s_min;
    memcpy(tmp, e->buf + s_min, n);
    tmp[n] = '\0';
    SetClipboardText(tmp);
    le_delete_selection(e);
    ret = true;
  }

  if (ctrl && IsKeyPressed(KEY_V)) {
    const char* cb = GetClipboardText();
    if (cb && *cb) {
      if (e->sel_anchor >= 0) le_delete_selection(e);
      static char tmp[LINEEDIT_BUFSIZE];
      int j = 0;
      for (const char* p = cb; *p && j < LINEEDIT_BUFSIZE - 1; p++) {
        if (*p != '\n' && *p != '\r') tmp[j++] = *p;
      }
      tmp[j] = '\0';
      int n = j;
      if (e->len + n < LINEEDIT_BUFSIZE) {
        memmove(e->buf + e->cursor + n, e->buf + e->cursor,
                e->len - e->cursor + 1);
        memcpy(e->buf + e->cursor, tmp, n);
        e->len += n;
        e->cursor += n;
        e->alive = 0;
        ret = true;
      }
    }
  }

  if (!ctrl) {
    int key = GetCharPressed();
    while (key > 0) {
      if (key >= 32 && key <= 126 && e->len < LINEEDIT_BUFSIZE - 1) {
        if (e->sel_anchor >= 0) le_delete_selection(e);
        memmove(e->buf + e->cursor + 1, e->buf + e->cursor,
                e->len - e->cursor + 1);
        e->buf[e->cursor] = (char)key;
        e->cursor++;
        e->len++;
        e->alive = 0;
        ret = true;
      }
      key = GetCharPressed();
    }
  }

  if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
    if (e->sel_anchor >= 0) {
      le_delete_selection(e);
    } else if (e->cursor > 0) {
      memmove(e->buf + e->cursor - 1, e->buf + e->cursor,
              e->len - e->cursor + 1);
      e->cursor--;
      e->len--;
      e->alive = 0;
    }
    ret = true;
  }

  if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
    if (e->sel_anchor >= 0) {
      le_delete_selection(e);
    } else if (e->cursor < e->len) {
      memmove(e->buf + e->cursor, e->buf + e->cursor + 1, e->len - e->cursor);
      e->len--;
      e->alive = 0;
    }
    ret = true;
  }

  if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
    if (!shift && e->sel_anchor >= 0) {
      e->cursor = min_int(e->cursor, e->sel_anchor);
      e->sel_anchor = -1;
    } else {
      if (shift && e->sel_anchor < 0) e->sel_anchor = e->cursor;
      if (e->cursor > 0) e->cursor--;
      if (!shift) e->sel_anchor = -1;
    }
    e->alive = 0;
  }

  if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
    if (!shift && e->sel_anchor >= 0) {
      e->cursor = max_int(e->cursor, e->sel_anchor);
      e->sel_anchor = -1;
    } else {
      if (shift && e->sel_anchor < 0) e->sel_anchor = e->cursor;
      if (e->cursor < e->len) e->cursor++;
      if (!shift) e->sel_anchor = -1;
    }
    e->alive = 0;
  }

  if (IsKeyPressed(KEY_HOME)) {
    if (shift && e->sel_anchor < 0) e->sel_anchor = e->cursor;
    e->cursor = 0;
    if (!shift) e->sel_anchor = -1;
    e->alive = 0;
  }

  if (IsKeyPressed(KEY_END)) {
    if (shift && e->sel_anchor < 0) e->sel_anchor = e->cursor;
    e->cursor = e->len;
    if (!shift) e->sel_anchor = -1;
    e->alive = 0;
  }

  le_scroll_to_cursor(e);
  return ret;
}

void lineedit_draw(LineEdit* e) {
  int lh = uifont_line_height();
  int pad = 8;

  Rectangle box = e->hitbox;
  int text_y = ((int)box.height - lh) / 2;

  // Background + border
  DrawRectangleRec(box, TEXT_BG);
  if (e->focused) {
    int bw = 2;
    Color border = CA_ORANGE;
    DrawRectangle((int)box.x, (int)box.y, (int)box.width, bw, border);
    DrawRectangle((int)box.x, (int)box.y + (int)box.height - bw, (int)box.width,
                  bw, border);
    DrawRectangle((int)box.x, (int)box.y, bw, (int)box.height, border);
    DrawRectangle((int)box.x + (int)box.width - bw, (int)box.y, bw,
                  (int)box.height, border);
  }

  BeginScissorMode((int)box.x + pad, (int)box.y, (int)box.width - 2 * pad,
                   (int)box.height);
  rlPushMatrix();
  rlTranslatef(box.x + pad - e->scroll_x, box.y, 0);

  // Selection highlight
  if (e->sel_anchor >= 0) {
    int s_min = min_int(e->cursor, e->sel_anchor);
    int s_max = max_int(e->cursor, e->sel_anchor);
    int x0 = le_x_of(e, s_min);
    int x1 = le_x_of(e, s_max);
    Color sel_col = get_lut_color(COLOR_ORANGE);
    DrawRectangle(x0, text_y, x1 - x0, lh, sel_col);
  }

  uifont_draw_texture(e->buf, 0, text_y, CA_WHITE);

  if (e->focused) {
    int cx = le_x_of(e, e->cursor);
    int blink = ((int)(3.0f * e->alive)) % 2;
    if (blink == 0) {
      DrawRectangle(cx, text_y, 1, lh, CA_WHITE);
    }
  }

  rlPopMatrix();
  EndScissorMode();
  draw_frame(e->hitbox);
}

// ---- MultiLineEdit ----

// Module-level line index (one MLE active at a time is fine for this game)
typedef struct {
  int start;
  int end;  // byte offset of the '\n' or EOF (exclusive of that char)
} MleLine;

static MleLine s_mle_lines[MLE_MAX_LINES];
static int s_mle_line_count = 0;

// MleLine.end is the exclusive upper bound of the visual line's content:
//   - hard newline: end = position of '\n', next line starts at end+1
//   - word wrap at space: end = position of ' ', next line starts at end+1
//   - hard wrap (no space): end = first char of next line, next line starts
//   at end
//   - EOF: end = len
// Cursor can be at [start .. end] for \n/space/EOF lines,
// and [start .. end-1] for hard-wrap lines (end belongs to next line).
static void mle_build_lines(MultiLineEdit* m) {
  s_mle_line_count = 0;
  int pos = 0;
  int text_w = m->text_w > 0 ? m->text_w : 9999;

  while (s_mle_line_count < MLE_MAX_LINES) {
    int line_start = pos;
    int last_space = -1;       // position of last ' ' seen on this visual line
    int last_space_next = -1;  // position after that space

    for (int i = pos;; i++) {
      if (i == m->len) {
        // EOF
        s_mle_lines[s_mle_line_count++] = (MleLine){line_start, m->len};
        pos = m->len + 1;
        goto done;
      }
      if (m->buf[i] == '\n') {
        s_mle_lines[s_mle_line_count++] = (MleLine){line_start, i};
        pos = i + 1;
        break;
      }

      // Measure width up to and including char i
      static char tmp[MLE_BUFSIZE];
      int n = i - line_start + 1;
      memcpy(tmp, m->buf + line_start, n);
      tmp[n] = '\0';
      int w = (int)uifont_text_size(tmp).x;

      if (w > text_w) {
        if (last_space >= 0) {
          // Wrap at last space (space not rendered, belongs to neither line)
          s_mle_lines[s_mle_line_count++] = (MleLine){line_start, last_space};
          pos = last_space_next;
        } else if (i > line_start) {
          // Hard wrap before char i
          s_mle_lines[s_mle_line_count++] = (MleLine){line_start, i};
          pos = i;
        } else {
          // Single char too wide — force it onto its own line
          s_mle_lines[s_mle_line_count++] = (MleLine){line_start, i + 1};
          pos = i + 1;
        }
        break;
      }

      if (m->buf[i] == ' ') {
        last_space = i;
        last_space_next = i + 1;
      }
    }

    if (pos > m->len) break;
  }

done:
  if (s_mle_line_count == 0) {
    s_mle_lines[0] = (MleLine){0, 0};
    s_mle_line_count = 1;
  }
}

// Returns the line index containing byte offset `pos`.
// Uses next line's start rather than current line's end to handle hard-wrap
// correctly (where end == next start).
static int mle_line_of(int pos) {
  for (int i = 0; i < s_mle_line_count - 1; i++) {
    if (pos < s_mle_lines[i + 1].start) return i;
  }
  return s_mle_line_count - 1;
}

// Returns x pixel offset of byte `pos` within its line (screen pixels)
static int mle_x_of(MultiLineEdit* m, int pos) {
  int li = mle_line_of(pos);
  int start = s_mle_lines[li].start;
  int n = pos - start;
  if (n <= 0) return 0;
  static char tmp[MLE_BUFSIZE];
  memcpy(tmp, m->buf + start, n);
  tmp[n] = '\0';
  return (int)uifont_text_size(tmp).x;
}

// Finds the byte offset in `line_idx` closest to x `target_x` (screen pixels)
static int mle_pos_from_x(MultiLineEdit* m, int line_idx, int target_x) {
  int start = s_mle_lines[line_idx].start;
  int end = s_mle_lines[line_idx].end;
  int best = start;
  int best_dist = abs(target_x);
  static char tmp[MLE_BUFSIZE];
  for (int pos = start + 1; pos <= end; pos++) {
    int n = pos - start;
    memcpy(tmp, m->buf + start, n);
    tmp[n] = '\0';
    int x = (int)uifont_text_size(tmp).x;
    int dist = abs(x - target_x);
    if (dist <= best_dist) {
      best = pos;
      best_dist = dist;
    } else {
      break;  // x is monotonically increasing
    }
  }
  return best;
}

static void mle_delete_selection(MultiLineEdit* m) {
  if (m->sel_anchor < 0) return;
  int s_min = min_int(m->cursor, m->sel_anchor);
  int s_max = max_int(m->cursor, m->sel_anchor);
  memmove(m->buf + s_min, m->buf + s_max, m->len - s_max + 1);
  m->len -= (s_max - s_min);
  m->cursor = s_min;
  m->sel_anchor = -1;
  m->alive = 0;
}

static void mle_scroll_to_cursor(MultiLineEdit* m) {
  int lh = uifont_line_height();
  int lh_total = lh + 2;
  int pad_s = m->pad;
  int cl = mle_line_of(m->cursor);
  int top = cl * lh_total;
  int bot = top + lh_total;
  int view_h = (int)m->hitbox.height;
  if (m->scroll.value > top + pad_s) m->scroll.value = top;
  if (m->scroll.value < pad_s + bot - view_h)
    m->scroll.value = pad_s + bot - view_h;
  if (m->scroll.value < 0) m->scroll.value = 0;
}

void mle_init(MultiLineEdit* m) {
  *m = (MultiLineEdit){0};
  m->sel_anchor = -1;
  m->pad = 8;
  scroll_init(&m->scroll);
}

void mle_set_box(MultiLineEdit* m, Rectangle box) {
  m->hitbox = box;
  scroll_set_content_box(&m->scroll, box, 24);
  m->text_w = (int)(box.width - 24) - 2 * m->pad;
}

void mle_set_text(MultiLineEdit* m, const char* txt) {
  if (!txt) txt = "";
  int n = (int)strlen(txt);
  if (n >= MLE_BUFSIZE) n = MLE_BUFSIZE - 1;
  memcpy(m->buf, txt, n);
  m->buf[n] = '\0';
  m->len = n;
  m->cursor = 0;
  m->sel_anchor = -1;
  m->alive = 0;
  mle_build_lines(m);
  m->line_count = s_mle_line_count;
  mle_scroll_to_cursor(m);
}

const char* mle_get_text(MultiLineEdit* m) { return m->buf; }

void mle_set_focus(MultiLineEdit* m, bool focused) {
  m->focused = focused;
  if (!focused) m->sel_anchor = -1;
}

bool mle_update(MultiLineEdit* m) {
  float dt = (float)ui_get_frame_time();
  m->alive += dt;
  int prev_cursor = m->cursor;

  mle_build_lines(m);
  m->line_count = s_mle_line_count;

  int lh = uifont_line_height();
  int lh_total = lh + 2;
  m->content_h = s_mle_line_count * lh_total + 2 * m->pad;

  Vector2 mouse = GetMousePosition();
  scroll_update(&m->scroll, m->content_h, mouse);

  bool hit_content = CheckCollisionPointRec(mouse, m->hitbox) && !m->scroll.hit;
  bool hit_box = CheckCollisionPointRec(mouse, m->hitbox);

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    m->dragging = false;
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hit_box) {
    mle_set_focus(m, false);
  }

  if (hit_content) {
    ui_inc_hit_count();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      mle_set_focus(m, true);
      bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
      int mx = (int)(mouse.x - m->hitbox.x - m->pad);
      int my = (int)(mouse.y - m->hitbox.y - m->pad + m->scroll.value);
      int li = my / lh_total;
      if (li < 0) li = 0;
      if (li >= s_mle_line_count) li = s_mle_line_count - 1;
      int clicked = mle_pos_from_x(m, li, mx);
      if (shift) {
        if (m->sel_anchor < 0) m->sel_anchor = m->cursor;
        m->cursor = clicked;
      } else {
        m->cursor = clicked;
        m->sel_anchor = -1;
        m->dragging = true;
      }
      m->alive = 0;
    }
  }

  if (m->dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    int mx = (int)(mouse.x - m->hitbox.x) - m->pad;
    int my = (int)(mouse.y - m->hitbox.y + m->scroll.value) - m->pad;
    int li = my / lh_total;
    if (li < 0) li = 0;
    if (li >= s_mle_line_count) li = s_mle_line_count - 1;
    int hovered = mle_pos_from_x(m, li, mx);
    if (hovered != m->cursor) {
      if (m->sel_anchor < 0) m->sel_anchor = m->cursor;
      m->cursor = hovered;
      m->alive = 0;
    }
  }

  if (!m->focused) return false;

  bool ret = false;
  bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  bool ctrl = is_control_down();

  // Ctrl+A: select all
  if (ctrl && IsKeyPressed(KEY_A)) {
    m->sel_anchor = 0;
    m->cursor = m->len;
    m->alive = 0;
  }

  // Ctrl+C: copy selection
  if (ctrl && IsKeyPressed(KEY_C) && m->sel_anchor >= 0) {
    int s_min = min_int(m->cursor, m->sel_anchor);
    int s_max = max_int(m->cursor, m->sel_anchor);
    static char tmp[MLE_BUFSIZE];
    int n = s_max - s_min;
    memcpy(tmp, m->buf + s_min, n);
    tmp[n] = '\0';
    SetClipboardText(tmp);
  }

  // Ctrl+X: cut selection
  if (!m->readonly && ctrl && IsKeyPressed(KEY_X) && m->sel_anchor >= 0) {
    int s_min = min_int(m->cursor, m->sel_anchor);
    int s_max = max_int(m->cursor, m->sel_anchor);
    static char tmp[MLE_BUFSIZE];
    int n = s_max - s_min;
    memcpy(tmp, m->buf + s_min, n);
    tmp[n] = '\0';
    SetClipboardText(tmp);
    mle_delete_selection(m);
    ret = true;
  }

  // Ctrl+V: paste
  if (!m->readonly && ctrl && IsKeyPressed(KEY_V)) {
    const char* cb = GetClipboardText();
    if (cb && *cb) {
      if (m->sel_anchor >= 0) mle_delete_selection(m);
      int n = (int)strlen(cb);
      if (m->len + n < MLE_BUFSIZE) {
        memmove(m->buf + m->cursor + n, m->buf + m->cursor,
                m->len - m->cursor + 1);
        memcpy(m->buf + m->cursor, cb, n);
        m->len += n;
        m->cursor += n;
        m->alive = 0;
        ret = true;
      }
    }
  }

  // Printable character input
  if (!m->readonly && !ctrl) {
    int key = GetCharPressed();
    while (key > 0) {
      if (key >= 32 && key <= 126 && m->len < MLE_BUFSIZE - 1) {
        if (m->sel_anchor >= 0) mle_delete_selection(m);
        memmove(m->buf + m->cursor + 1, m->buf + m->cursor,
                m->len - m->cursor + 1);
        m->buf[m->cursor] = (char)key;
        m->cursor++;
        m->len++;
        m->alive = 0;
        ret = true;
      }
      key = GetCharPressed();
    }
  }

  // Enter (insert newline)
  if (!m->readonly && IsKeyPressed(KEY_ENTER) && m->len < MLE_BUFSIZE - 1) {
    if (m->sel_anchor >= 0) mle_delete_selection(m);
    memmove(m->buf + m->cursor + 1, m->buf + m->cursor, m->len - m->cursor + 1);
    m->buf[m->cursor] = '\n';
    m->cursor++;
    m->len++;
    m->alive = 0;
    ret = true;
  }

  // Backspace
  if (!m->readonly &&
      (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))) {
    if (m->sel_anchor >= 0) {
      mle_delete_selection(m);
    } else if (m->cursor > 0) {
      memmove(m->buf + m->cursor - 1, m->buf + m->cursor,
              m->len - m->cursor + 1);
      m->cursor--;
      m->len--;
      m->alive = 0;
    }
    ret = true;
  }

  // Delete
  if (!m->readonly &&
      (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE))) {
    if (m->sel_anchor >= 0) {
      mle_delete_selection(m);
    } else if (m->cursor < m->len) {
      memmove(m->buf + m->cursor, m->buf + m->cursor + 1, m->len - m->cursor);
      m->len--;
      m->alive = 0;
    }
    ret = true;
  }

  // Rebuild lines after potential text modifications
  mle_build_lines(m);
  m->line_count = s_mle_line_count;

  // Left
  if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
    if (!shift && m->sel_anchor >= 0) {
      m->cursor = min_int(m->cursor, m->sel_anchor);
      m->sel_anchor = -1;
    } else {
      if (shift && m->sel_anchor < 0) m->sel_anchor = m->cursor;
      if (m->cursor > 0) m->cursor--;
      if (!shift) m->sel_anchor = -1;
    }
    m->alive = 0;
  }

  // Right
  if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
    if (!shift && m->sel_anchor >= 0) {
      m->cursor = max_int(m->cursor, m->sel_anchor);
      m->sel_anchor = -1;
    } else {
      if (shift && m->sel_anchor < 0) m->sel_anchor = m->cursor;
      if (m->cursor < m->len) m->cursor++;
      if (!shift) m->sel_anchor = -1;
    }
    m->alive = 0;
  }

  // Up
  if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
    if (shift && m->sel_anchor < 0) m->sel_anchor = m->cursor;
    int cl = mle_line_of(m->cursor);
    if (cl > 0) {
      int col = m->cursor - s_mle_lines[cl].start;
      int prev_len = s_mle_lines[cl - 1].end - s_mle_lines[cl - 1].start;
      m->cursor = s_mle_lines[cl - 1].start + min_int(col, prev_len);
    }
    if (!shift) m->sel_anchor = -1;
    m->alive = 0;
  }

  // Down
  if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
    if (shift && m->sel_anchor < 0) m->sel_anchor = m->cursor;
    int cl = mle_line_of(m->cursor);
    if (cl < s_mle_line_count - 1) {
      int col = m->cursor - s_mle_lines[cl].start;
      int next_len = s_mle_lines[cl + 1].end - s_mle_lines[cl + 1].start;
      m->cursor = s_mle_lines[cl + 1].start + min_int(col, next_len);
    }
    if (!shift) m->sel_anchor = -1;
    m->alive = 0;
  }

  // Home
  if (IsKeyPressed(KEY_HOME)) {
    if (shift && m->sel_anchor < 0) m->sel_anchor = m->cursor;
    m->cursor = s_mle_lines[mle_line_of(m->cursor)].start;
    if (!shift) m->sel_anchor = -1;
    m->alive = 0;
  }

  // End
  if (IsKeyPressed(KEY_END)) {
    if (shift && m->sel_anchor < 0) m->sel_anchor = m->cursor;
    m->cursor = s_mle_lines[mle_line_of(m->cursor)].end;
    if (!shift) m->sel_anchor = -1;
    m->alive = 0;
  }

  if (m->cursor != prev_cursor) mle_scroll_to_cursor(m);
  return ret;
}

void mle_draw(MultiLineEdit* m) {
  int lh = uifont_line_height();
  int lh_total = lh + 2;

  mle_build_lines(m);

  int sel_min = -1, sel_max = -1;
  if (m->sel_anchor >= 0) {
    sel_min = min_int(m->cursor, m->sel_anchor);
    sel_max = max_int(m->cursor, m->sel_anchor);
  }
  int cursor_line = mle_line_of(m->cursor);

  Rectangle box = m->hitbox;

  DrawRectangleRec(box, TEXT_BG);
  if (m->focused) {
    int bw = 2;
    Color border = CA_ORANGE;
    DrawRectangle((int)box.x, (int)box.y, (int)box.width, bw, border);
    DrawRectangle((int)box.x, (int)box.y + (int)box.height - bw, (int)box.width,
                  bw, border);
    DrawRectangle((int)box.x, (int)box.y, bw, (int)box.height, border);
    DrawRectangle((int)box.x + (int)box.width - bw, (int)box.y, bw,
                  (int)box.height, border);
  }

  BeginScissorMode((int)box.x, (int)box.y, (int)box.width, (int)box.height);
  rlPushMatrix();
  rlTranslatef(box.x, box.y, 0);
  rlTranslatef(m->pad, m->pad - m->scroll.value, 0);

  static char tmp[MLE_BUFSIZE];

  for (int i = 0; i < s_mle_line_count; i++) {
    int y = i * lh_total;
    int line_start = s_mle_lines[i].start;
    int line_end = s_mle_lines[i].end;

    // Selection highlight
    if (sel_min >= 0) {
      int hs = max_int(sel_min, line_start);
      int he = min_int(sel_max, line_end);
      bool extends = sel_max > line_end;
      if (hs <= he || extends) {
        int xstart = 0;
        if (hs > line_start) {
          int n = hs - line_start;
          memcpy(tmp, m->buf + line_start, n);
          tmp[n] = '\0';
          xstart = (int)uifont_text_size(tmp).x;
        }
        int xend;
        if (he >= line_end) {
          int n = line_end - line_start;
          memcpy(tmp, m->buf + line_start, n);
          tmp[n] = '\0';
          xend = (int)uifont_text_size(tmp).x;
          if (extends) xend += 8;  // show newline char selected
        } else {
          int n = he - line_start;
          memcpy(tmp, m->buf + line_start, n);
          tmp[n] = '\0';
          xend = (int)uifont_text_size(tmp).x;
        }
        Color sel_col = get_lut_color(COLOR_ORANGE);
        DrawRectangle(xstart, y, xend - xstart, lh, sel_col);
      }
    }

    // Draw line text
    int n = line_end - line_start;
    memcpy(tmp, m->buf + line_start, n);
    tmp[n] = '\0';
    uifont_draw_texture(tmp, 0, y, CA_WHITE);

    // Draw cursor
    if (i == cursor_line && m->focused) {
      int col = m->cursor - line_start;
      memcpy(tmp, m->buf + line_start, col);
      tmp[col] = '\0';
      int cx = (int)uifont_text_size(tmp).x;
      int blink = ((int)(3.0f * m->alive)) % 2;
      if (blink == 0) {
        DrawRectangle(cx, y, 1, lh, CA_WHITE);
      }
    }
  }

  rlPopMatrix();
  EndScissorMode();
  scroll_draw(&m->scroll);
  draw_frame(m->hitbox);
}

static void draw_checkbox(Rectangle r, bool toggled, bool pressed) {
  rlPushMatrix();
  rlTranslatef(r.x, r.y, 0);
  rlScalef(2, 2, 1);
  int ww = r.width / 2;
  int hh = r.height / 2;
  NPatchInfo patch_normal = {.source = {528, 96, 5, 5},
                             .left = 2,
                             .top = 2,
                             .right = 2,
                             .bottom = 2,
                             .layout = NPATCH_NINE_PATCH};
  NPatchInfo patch_pressed = {.source = {528 + 5, 96, 5, 5},
                              .left = 2,
                              .top = 2,
                              .right = 2,
                              .bottom = 2,
                              .layout = NPATCH_NINE_PATCH};

  Rectangle dest = {0, 0, ww, hh};
  Texture sprites = ui_get_sprites();
  DrawTextureNPatch(sprites, pressed ? patch_pressed : patch_normal, dest,
                    (Vector2){0, 0}, 0, WHITE);

  if (toggled) {
    int cx = (ww - 13) / 2;
    int cy = (hh - 13) / 2;
    DrawTextureRec(sprites, rect_check, (Vector2){cx, cy}, BLACK);
  }
  rlPopMatrix();
}

void btn_draw_checkbox(Btn* b) {
  if (b->hidden) return;
  draw_checkbox(b->hitbox, b->toggled, b->pressed);
}

void btn_draw_checkbox_text(Btn* b, const char* txt) {
  int h = b->hitbox.height;
  int p = 4;
  if (b->hover) {
    Color bg = TEXT_BG;
    DrawRectangleRec(b->hitbox, bg);
  }
  Rectangle r_box = {b->hitbox.x + p, b->hitbox.y + p, h - 2 * p, h - 2 * p};
  draw_checkbox(r_box, b->toggled, b->pressed);
  int sep = 8;
  Rectangle r_lab = {b->hitbox.x + h + sep, b->hitbox.y, h, h};

  int lh = uifont_line_height();
  int yy = (h - lh) / 2;

  rlPushMatrix();
  rlTranslatef(r_lab.x, r_lab.y + yy, 0);
  uifont_draw_texture(txt, 2, 2, BLACK);
  uifont_draw_texture(txt, 0, 0, CA_WHITE);
  rlPopMatrix();
}

bool btn_right_click(Btn* b) {
  return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && b->hover && !b->disabled;
}
