#include "widgets.h"

#include <math.h>
#include <stb_ds.h>
#include <stdlib.h>

#include "assert.h"
#include "colors.h"
#include "font.h"
#include "ui.h"
#include "utils.h"
#include "wmain.h"

static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

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
  int row_height = get_font_line_height() + 2 * l->row_pad;
  row.sprite = sprite;
  row.text_y = l->row_pad;
  row.content = clone_string(content);
  row.hitbox_r = (Rectangle){
      0,
      y,
      w,
      row_height * 2,
  };
  l->height += row.hitbox_r.height;
  arrput(l->rows, row);
}

void listbox_add_row(Listbox* l, const char* content) {
  listbox_add_row_icon(l, content, (sprite_t){0});
}

static void listbox_update_row_sizes(Listbox* l) {
  l->height = 0;
  int s = 2;
  for (int i = 0; i < arrlen(l->rows); i++) {
    int w = l->hitbox.width;
    int y = l->height;
    int row_height = get_font_line_height() + 2 * l->row_pad;
    l->rows[i].text_y = l->row_pad;
    l->rows[i].hitbox_r = (Rectangle){
        0,
        y,
        w,
        row_height * s,
    };
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
      if (hit) {
        l->row_hit = i;
        break;
      }
    }
  }
}

static void draw_row(int row_w, int row_h, ListboxRow* row, bool sel) {
  Color bg = BLANK;
  Color fg = WHITE;
  if (sel) {
    bg = get_lut_color(COLOR_ORANGE);
    fg = BLACK;
  }
  DrawRectangle(0, 0, row_w, row_h, bg);
  rlPushMatrix();
  int s = ui_get_scale();
  rlScalef(s, s, 1);
  if (row->sprite.region.width > 0) {
    int tw = row->sprite.region.width;
    int th = row->sprite.region.height;
    int y = (row_h / 2 - th) / 2;
    DrawTexture(row->sprite.tex, 2, y, fg);
    rlTranslatef(2 + 4 + tw, 0, 0);
  }
  int pad = 2;
  font_draw_texture(row->content, pad, row->text_y, fg);
  rlPopMatrix();
}

void listbox_draw(Listbox* l, int selected) {
  Color bg = WHITE;
  int s = ui_get_scale();
  bg.a = 20;
  Rectangle box = l->hitbox;
  BeginScissorMode(box.x, box.y, box.width, box.height);
  rlPushMatrix();
  rlTranslatef(box.x, box.y - l->scroll.value, 0);
  Color c = {0, 0, 0, 150};
  DrawRectangle(0, 0, box.width * s, box.height * s, c);
  for (int i = 0; i < arrlen(l->rows); i++) {
    ListboxRow* row = &l->rows[i];
    int row_h = row->hitbox_r.height;
    int row_w = row->hitbox_r.width;
    draw_row(row_w, row_h, row, selected == i);
    rlTranslatef(0, row->hitbox_r.height, 0);
  }
  rlPopMatrix();
  scroll_draw(&l->scroll);
  EndScissorMode();
  // draw_widget_frame(box);
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
    b.hitbox.width -= 2;
    b.pressed = s->down;
    btn_draw_text(&b, 2, "");
  }
  rlPopMatrix();
}

void textbox_set_bg(Textbox* t, Color bg) { t->bg = bg; }

void textbox_init(Textbox* t) {
  *t = (Textbox){0};
  // t->bg = BLACK;
  t->bg = (Color){0, 0, 0, 150};
  scroll_init(&t->scroll);
}

void textbox_calc_height(Textbox* t) {
  /* Computes text height and width */
  if (!t->text) return;
  int scale = ui_get_scale();
  t->text_x = 12 * scale;
  t->text_w =
      (t->box.width - t->scroll.scroll_rect.width - 2 * t->text_x) / scale;
  int th;
  draw_text_box_advanced(t->text, (Rectangle){0, 0, t->text_w, 0}, WHITE,
                         t->sprites, &th);
  t->height = 2 * th;

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
  int s = ui_get_scale();
  BeginScissorMode(box.x, box.y, box.width, box.height);
  rlPushMatrix();
  rlTranslatef(box.x, box.y, 0);
  DrawRectangle(0, 0, box.width, t->box.height, t->bg);
  rlTranslatef(t->text_x, -t->scroll.value, 0);
  rlScalef(s, s, 1);
  draw_text_box_advanced(t->text, (Rectangle){0, 0, t->text_w, 0}, WHITE,
                         t->sprites, NULL);

  rlPopMatrix();
  EndScissorMode();
  scroll_draw(&t->scroll);
}

void textbox_destroy(Textbox* t) {
  if (t->text) free(t->text);
}

void textbox_set_box(Textbox* t, Rectangle box) {
  t->box = box;
  scroll_set_content_box(&t->scroll, box, 24);
}

// Updates button state. Returns true if button was clicked.
bool btn_update(Btn* b) {
  if (b->disabled || b->hidden) return false;
  Vector2 pos = GetMousePosition();
  bool hit = CheckCollisionPointRec(pos, b->hitbox) && ui_get_hit_count() == 0;
  if (hit) {
    ui_inc_hit_count();
  }

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
  b->hover = hit;
  if (ret) {
    on_click();
  }
  return ret;
}

// Text button (close button for example).
void btn_draw_text(Btn* b, int ui_scale, const char* text) {
  if (b->hidden) return;
  Vector2 size = get_rendered_text_size(text);
  int fx = (b->hitbox.x + b->hitbox.width / 2) / ui_scale - size.x / 2;
  int fy = (b->hitbox.y + b->hitbox.height / 2) / ui_scale - size.y / 2 + 1;
  if (b->pressed) fy += 1;

  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  int s = ui_scale;
  Color c0 = get_lut_color(COLOR_BTN0);
  Color c1 = get_lut_color(COLOR_BTN1);
  Color c2 = get_lut_color(COLOR_BTN2);
  Color cbg = get_lut_color(COLOR_BTN_BG);
  DrawRectangle(x - s, y - s, w + 2 * s, h + 2 * s, cbg);
  DrawRectangle(x, y, w, h, c1);
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    DrawRectangle(x, y, w, s, c0);  // TOP
    DrawRectangle(x, y, s, h, c0);  // LEFT
  } else {
    DrawRectangle(x, y, w - s, s, c2);
    DrawRectangle(x, y, s, h - s, c2);
  }
  DrawRectangle(x + s, y + h - s, w - s, s, c0);
  DrawRectangle(x + w - s, y + s, s, h - s, c0);

  rlPushMatrix();
  rlScalef(ui_scale, ui_scale, 1);

  if (!b->disabled) {
    font_draw_texture(text, fx, fy, BLACK);
  } else {
    font_draw_texture(text, fx + 1, fy, c2);
    font_draw_texture(text, fx, fy + 1, c2);
    font_draw_texture(text, fx + 1, fy + 1, c2);
    font_draw_texture(text, fx, fy, c0);
  }

  rlPopMatrix();
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    Color bl = BLACK;
    bl.a = 100;
    DrawRectangle(x, y, w, h, bl);
  }
}

void btn_draw_text_primary(Btn* b, int ui_scale, const char* text) {
  if (b->hidden) return;
  Vector2 size = get_rendered_text_size(text);
  int fx = (b->hitbox.x + b->hitbox.width / 2) / ui_scale - size.x / 2;
  int fy = (b->hitbox.y + b->hitbox.height / 2) / ui_scale - size.y / 2 + 1;
  if (b->pressed) fy += 1;

  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  int s = ui_scale;
  //  0x802601FF Da
  Color darker = GetColor(0x802601FF);
  Color bg1 = GetColor(0Xe88038FF);  // GetColor(0XFC6700FF);
  Color fg = GetColor(0xFDD900FF);

  Color c0 = darker;  // GetColor(0x6C6C53FF);
  Color c1 = bg1;     // GetColor(0x6C6C53FF);
  Color c2 = fg;      // GetColor(0x6C6C53FF);
  Color cbg = get_lut_color(COLOR_BTN_BG);
  DrawRectangle(x - s, y - s, w + 2 * s, h + 2 * s, cbg);
  DrawRectangle(x, y, w, h, c1);
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    DrawRectangle(x, y, w, s, c0);  // TOP
    DrawRectangle(x, y, s, h, c0);  // LEFT
  } else {
    DrawRectangle(x, y, w - s, s, c2);
    DrawRectangle(x, y, s, h - s, c2);
  }
  DrawRectangle(x + s, y + h - s, w - s, s, c0);
  DrawRectangle(x + w - s, y + s, s, h - s, c0);

  rlPushMatrix();
  rlScalef(ui_scale, ui_scale, 1);

  if (!b->disabled) {
    font_draw_texture(text, fx, fy, BLACK);
  } else {
    font_draw_texture(text, fx + 1, fy, c2);
    font_draw_texture(text, fx, fy + 1, c2);
    font_draw_texture(text, fx + 1, fy + 1, c2);
    font_draw_texture(text, fx, fy, c0);
  }

  rlPopMatrix();
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    Color bl = BLACK;
    bl.a = 100;
    DrawRectangle(x, y, w, h, bl);
  }
}

// Icon button: tool buttons and challenge button.
void btn_draw_icon(Btn* b, int ui_scale, Texture2D texture, Rectangle source) {
  if (b->hidden) return;
  int fx = (b->hitbox.x + b->hitbox.width / 2) / ui_scale - source.width / 2;
  int fy = (b->hitbox.y + b->hitbox.height / 2) / ui_scale - source.height / 2;
  if (b->pressed) fy += 1;
  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  int s = ui_scale;
  Color c0 = get_lut_color(COLOR_BTN0);
  Color c1 = get_lut_color(COLOR_BTN1);
  Color c2 = get_lut_color(COLOR_BTN2);
  Color cbg = get_lut_color(COLOR_BTN_BG);
  DrawRectangle(x - s, y - s, w + 2 * s, h + 2 * s, cbg);

  if (b->gradient) {
    Color top = GetColor(0xF4D949FF);
    Color bottom = GetColor(0x762E10FF);
    DrawRectangleGradientV(x, y, w, h, top, bottom);
  } else {
    DrawRectangle(x, y, w, h, c1);
  }
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    DrawRectangle(x, y, w, s, c0);  // TOP
    DrawRectangle(x, y, s, h, c0);  // LEFT
  } else {
    DrawRectangle(x, y, w - s, s, c2);  // TOP
    DrawRectangle(x, y, s, h - s, c2);  // LEFT
  }
  DrawRectangle(x + s, y + h - s, w - s, s, c0);
  DrawRectangle(x + w - s, y + s, s, h - s, c0);
  rlPushMatrix();
  rlScalef(ui_scale, ui_scale, 1);
  if (!b->disabled) {
    if (!b->gradient) {
      DrawTextureRec(texture, source, (Vector2){fx, fy}, BLACK);
    } else {
      DrawTextureRec(texture, source, (Vector2){fx, fy + 1}, BLACK);
      DrawTextureRec(texture, source, (Vector2){fx, fy}, WHITE);
    }
  } else {
    DrawTextureRec(texture, source, (Vector2){fx + 1, fy}, c2);
    DrawTextureRec(texture, source, (Vector2){fx, fy + 1}, c2);
    DrawTextureRec(texture, source, (Vector2){fx + 1, fy + 1}, c2);
    DrawTextureRec(texture, source, (Vector2){fx, fy}, c0);
  }
  rlPopMatrix();
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    Color bl = BLACK;
    bl.a = 100;
    DrawRectangle(x, y, w, h, bl);
  }
}

// Draws the legend of the button. Needs to be called separately from the button
// itself.
void btn_draw_legend(Btn* b, int ui_scale, const char* text) {
  Color k = {21, 11, 3, 255};
  if (b->hidden) return;
  Vector2 pos = GetMousePosition();
  bool hover = CheckCollisionPointRec(pos, b->hitbox);
  if (hover) {
    int x = pos.x + 16;
    int y = pos.y + 16;
    Color kk = get_lut_color(COLOR_BG0);
    Color fc = get_lut_color(COLOR_FC0);
    x = (x / ui_scale) * ui_scale;
    y = (y / ui_scale) * ui_scale;
    int s = 2;
    int bh, bw;
    get_draw_text_box_size(text, 500, &bh, &bw);
    Rectangle area = {x, y, ui_scale * (bw + 6), ui_scale * (bh + 6)};
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
    // DrawRectangleRec(area, k);
    rlPushMatrix();
    rlScalef(ui_scale, ui_scale, 1);
    y = y / ui_scale + 4;
    x = x / ui_scale + 4;
    draw_text_box(text, (Rectangle){x, y, 500, 0}, fc, NULL);
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
  int s = ui_get_scale();
  rlPushMatrix();
  int lh = get_font_line_height();
  int xx = 2;
  int yy = (b->hitbox.height - lh) / 2;
  rlTranslatef(b->hitbox.x + xx, b->hitbox.y + yy, 0);
  rlScalef(s, s, 1);

  // Draw text before cursor
  char temp[256];
  strncpy(temp, b->txt, b->cursor_pos);
  temp[b->cursor_pos] = '\0';
  int cursor_x = get_rendered_text_size(temp).x;

  // Draw full text
  font_draw_texture(b->txt, 1, 1, BLACK);
  font_draw_texture(b->txt, 0, 0, WHITE);

  // Draw cursor at cursor_pos
  int cursor_type = ((int)(3 * b->alive)) % 2;
  if (cursor_type == 0) {
    rlTranslatef(cursor_x + 1, 0, 0);
    DrawRectangle(0, 0, 1 + 1, 7 + 1, BLACK);
    DrawRectangle(0, 0, 1, 7, WHITE);
  }
  rlPopMatrix();
}

void textbox_scroll_to_bottom(Textbox* t) {
  scroll_scroll_to_bottom(&t->scroll, t->height);
}

void label_set_text(Label* l, const char* txt) {
  strncpy(l->txt, txt, sizeof(l->txt));
}

void label_draw(Label* l) {
  rlPushMatrix();
  rlTranslatef(l->hitbox.x, l->hitbox.y, 0);
  int th = 20;
  int lh = get_font_line_height();
  int offy = (th - lh) / 2 + 2;
  int offx = 5;
  int s = 2;
  rlScalef(s, s, 1);
  font_draw_texture_outlined(l->txt, offx, offy, WHITE, BLACK);
  rlPopMatrix();
}
