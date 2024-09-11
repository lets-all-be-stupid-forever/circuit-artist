#include "widgets.h"

#include "colors.h"
#include "font.h"
#include "math.h"
#include "rlgl.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"
#include "utils.h"

void ListboxLoad(Listbox* l)
{
  *l = (Listbox){0};
  ScrollLoad(&l->scroll);
}

void ListboxSetBox(Listbox* l, Rectangle r)
{
  l->hitbox = r;
  ScrollSetContentBox(&l->scroll, r, 10);
}

void ListboxAddRow(Listbox* l, const char* content)
{
  ListboxRow row = {0};
  int w = l->hitbox.width;
  int y = l->height;
  int row_height = GetFontLineHeight() + 8;
  row.text_y = 4;
  row.content = CloneString(content);
  row.hitbox_r = (Rectangle){
      0,
      y,
      w,
      row_height * 2,
  };
  l->height += row.hitbox_r.height;
  arrput(l->rows, row);
}

static void ListboxUpdateRowSizes(Listbox* l)
{
  l->height = 0;
  int s = 2;
  for (int i = 0; i < arrlen(l->rows); i++) {
    int w = l->hitbox.width;
    int y = l->height;
    int row_height = GetFontLineHeight() + 8;
    l->rows[i].text_y = 4;
    l->rows[i].hitbox_r = (Rectangle){
        0,
        y,
        w,
        row_height * s,
    };
    l->height += l->rows[i].hitbox_r.height;
  }
}

void ListboxUpdate(Listbox* l)
{
  Vector2 mouse = GetMousePosition();
  ListboxUpdateRowSizes(l);
  int sy = -l->scroll.value;
  l->row_hit = -1;
  Vector2 mouse_ui = {mouse.x, mouse.y};
  ScrollUpdate(&l->scroll, l->height, mouse_ui);
  // TODO: it should affect the hitcount in Ui*
  bool content_hit = CheckCollisionPointRec(mouse_ui, l->hitbox);
  if (content_hit && !l->scroll.hit) {
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

void ListboxDraw(Listbox* l, Ui* ui, int selected)
{
  int s = ui->scale;
  Color bg = WHITE;
  bg.a = 20;
  Rectangle box = l->hitbox;
  BeginScissorMode(box.x, box.y, box.width, box.height);
  rlPushMatrix();
  rlTranslatef(box.x, box.y - l->scroll.value, 0);
  DrawRectangle(0, 0, box.width * s, box.height * s, BLACK);

  int pad = 2;
  for (int i = 0; i < arrlen(l->rows); i++) {
    ListboxRow* row = &l->rows[i];
    int row_h = row->hitbox_r.height;
    int row_w = row->hitbox_r.width;
    if (selected == i) {
      Color kk = GetLutColor(COLOR_ORANGE);
      DrawRectangle(0, 0, row_w, row_h, kk);
      rlPushMatrix();
      rlScalef(s, s, 1);
      FontDrawTexture(row->content, pad, row->text_y, BLACK);
      rlPopMatrix();
    }
    else {
      DrawRectangle(0, 0, row_w, row_h, BLACK);
      rlPushMatrix();
      rlScalef(s, s, 1);
      FontDrawTexture(row->content, pad, row->text_y, WHITE);
      rlPopMatrix();
    }
    rlTranslatef(0, row->hitbox_r.height, 0);
  }
  rlPopMatrix();
  ScrollDraw(&l->scroll);
  EndScissorMode();
}

// scroll I need: 1. value, some state for hovering: mouse0, value0
// value, mouse0, value0

void ListboxUnload(Listbox* l)
{
  for (int i = 0; i < arrlen(l->rows); i++) {
    free(l->rows[i].content);
  }
  arrfree(l->rows);
  *l = (Listbox){0};
}

void ScrollLoad(Scroll* s)
{
  *s = (Scroll){0};
}

void ScrollSetContentBox(Scroll* s, Rectangle box, int w)
{
  s->content = box;
  s->scroll_rect = (Rectangle){
      box.x + box.width - w,
      box.y,
      w,
      box.height};
}

void ScrollResetValue(Scroll* s)
{
  s->value = 0;
}

int ScrollGetValue(Scroll* s)
{
  return s->value;
}

static int FClip(int s, int vmin, int vmax)
{
  if (s < vmin) return vmin;
  if (s > vmax) return vmax;
  return s;
}

void ScrollUpdate(Scroll* s, float h, Vector2 mouse)
{
  int th = s->scroll_rect.height;
  int max_value = FClip(h - th, 0, 100000);
  s->hit = false;
  if (h < s->content.height) {
    s->hidden = true;
    return;
  }
  else {
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
  if (CheckCollisionPointRec(mouse, s->scroll_rect)) {
    if (CheckCollisionPointRec(mouse, s->rect)) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        s->down = true;
        s->mouse0 = mouse.y;
        s->value0 = s->value;
      }
    }
  }
  s->value = FClip(s->value, 0, max_value);
  int srect = (th * th) / h;
  int y = (s->value * th) / h;
  if (srect < th) {
    s->rect.x = s->scroll_rect.x;
    s->rect.y = s->scroll_rect.y + y;
    s->rect.width = s->scroll_rect.width;
    s->rect.height = srect;
  }
  else {
    s->hidden = true;
    s->rect.width = 0;
    s->rect.height = 0;
  }
}

void ScrollDraw(Scroll* s)
{
  rlPushMatrix();
  if (!s->hidden) {
    Color bg_color = GetLutColor(COLOR_DARKGRAY);
    DrawRectangleRec(s->scroll_rect, bg_color);
    Color c1 = GetLutColor(COLOR_BTN1);
    if (s->down) {
      c1 = GetLutColor(COLOR_ORANGE);
    }
    DrawRectangleRec(s->rect, c1);
  }
  rlPopMatrix();
}

void TextboxLoad(Textbox* t)
{
  *t = (Textbox){0};
  ScrollLoad(&t->scroll);
}

void TextboxSetContent(Textbox* t, const char* txt, Sprite* sprites)
{
  if (t->text) {
    free(t->text);
  }
  t->text = CloneString(txt);
  t->sprites = sprites;
  t->height = 0;
  ScrollResetValue(&t->scroll);
}

void TextboxUpdate(Textbox* t, Ui* ui)
{
  t->text_x = 12 * ui->scale;
  t->text_w = (t->box.width - t->scroll.scroll_rect.width - 2 * t->text_x) / ui->scale;
  if (t->height == 0) {
    int th;
    DrawTextBoxAdvanced(t->text, (Rectangle){0, 0, t->text_w, 0}, WHITE, t->sprites, &th);
    t->height = 2 * th;
  }
  Vector2 mouse = GetMousePosition();
  ScrollUpdate(&t->scroll, t->height, mouse);
  if (CheckCollisionPointRec(mouse, t->box)) {
    ui->hit_count++;
  }
}

void TextboxDraw(Textbox* t, Ui* ui)
{
  Rectangle box = t->box;
  int s = ui->scale;
  BeginScissorMode(box.x, box.y, box.width, box.height);
  rlPushMatrix();
  rlTranslatef(box.x, box.y, 0);
  DrawRectangle(0, 0, box.width, t->box.height, BLACK);
  rlTranslatef(t->text_x, -t->scroll.value, 0);
  rlScalef(s, s, 1);
  DrawTextBoxAdvanced(t->text, (Rectangle){0, 0, t->text_w, 0}, WHITE, t->sprites, NULL);

  rlPopMatrix();
  EndScissorMode();
  ScrollDraw(&t->scroll);
}

void TextboxUnload(Textbox* t)
{
  if (t->text) free(t->text);
}

void TextboxSetBox(Textbox* t, Rectangle box)
{
  t->box = box;
  ScrollSetContentBox(&t->scroll, box, 30);
}

bool BtnUpdate(Btn* b, Ui* ui)
{
  if (b->disabled || b->hidden) return false;
  Vector2 pos = GetMousePosition();
  bool hit = CheckCollisionPointRec(pos, b->hitbox) && ui->hit_count == 0;
  if (hit) {
    ui->hit_count++;
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
  return ret;
}

void BtnDrawText(Btn* b, int ui_scale, const char* text)
{
  Vector2 size = GetRenderedTextSize(text);
  int fx = (b->hitbox.x + b->hitbox.width / 2) / ui_scale - size.x / 2;
  int fy = (b->hitbox.y + b->hitbox.height / 2) / ui_scale - size.y / 2 + 1;
  if (b->pressed) fy += 1;

  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  int s = ui_scale;
  Color c0 = GetLutColor(COLOR_BTN0);
  Color c1 = GetLutColor(COLOR_BTN1);
  Color c2 = GetLutColor(COLOR_BTN2);
  Color cbg = GetLutColor(COLOR_BTN_BG);
  DrawRectangle(x - s, y - s, w + 2 * s, h + 2 * s, cbg);
  DrawRectangle(x, y, w, h, c1);
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    DrawRectangle(x, y, w, s, c0);  // TOP
    DrawRectangle(x, y, s, h, c0);  // LEFT
  }
  else {
    DrawRectangle(x, y, w - s, s, c2);
    DrawRectangle(x, y, s, h - s, c2);
  }
  DrawRectangle(x + s, y + h - s, w - s, s, c0);
  DrawRectangle(x + w - s, y + s, s, h - s, c0);

  rlPushMatrix();
  rlScalef(ui_scale, ui_scale, 1);

  if (!b->disabled) {
    FontDrawTexture(text, fx, fy, BLACK);
  }
  else {
    FontDrawTexture(text, fx + 1, fy, c2);
    FontDrawTexture(text, fx, fy + 1, c2);
    FontDrawTexture(text, fx + 1, fy + 1, c2);
    FontDrawTexture(text, fx, fy, c0);
  }

  rlPopMatrix();
  if (b->pressed) {
    Color bl = BLACK;
    bl.a = 50;
    DrawRectangle(x, y, w, h, bl);
  }
}

void BtnDrawIcon(Btn* b, int ui_scale, Texture2D texture, Rectangle source)
{
  if (b->hidden) return;
  int fx = (b->hitbox.x + b->hitbox.width / 2) / ui_scale - source.width / 2;
  int fy = (b->hitbox.y + b->hitbox.height / 2) / ui_scale - source.height / 2;
  if (b->pressed) fy += 1;
  int x = b->hitbox.x;
  int y = b->hitbox.y;
  int w = b->hitbox.width;
  int h = b->hitbox.height;
  int s = ui_scale;
  Color c0 = GetLutColor(COLOR_BTN0);
  Color c1 = GetLutColor(COLOR_BTN1);
  Color c2 = GetLutColor(COLOR_BTN2);
  Color cbg = GetLutColor(COLOR_BTN_BG);
  DrawRectangle(x - s, y - s, w + 2 * s, h + 2 * s, cbg);
  DrawRectangle(x, y, w, h, c1);
  if ((b->pressed || b->toggled) && (!b->disabled)) {
    DrawRectangle(x, y, w, s, c0);  // TOP
    DrawRectangle(x, y, s, h, c0);  // LEFT
  }
  else {
    DrawRectangle(x, y, w - s, s, c2);  // TOP
    DrawRectangle(x, y, s, h - s, c2);  // LEFT
  }
  DrawRectangle(x + s, y + h - s, w - s, s, c0);
  DrawRectangle(x + w - s, y + s, s, h - s, c0);
  rlPushMatrix();
  rlScalef(ui_scale, ui_scale, 1);
  if (!b->disabled) {
    DrawTextureRec(texture, source, (Vector2){fx, fy}, BLACK);
  }
  else {
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

void BtnDrawLegend(Btn* b, int ui_scale, const char* text)
{
  if (b->hidden) return;
  Vector2 pos = GetMousePosition();
  bool hover = CheckCollisionPointRec(pos, b->hitbox);
  if (hover) {
    int x = pos.x + 16;
    int y = pos.y + 16;
    Color kk = GetLutColor(COLOR_BG0);
    Color fc = GetLutColor(COLOR_FC0);
    x = (x / ui_scale) * ui_scale;
    y = (y / ui_scale) * ui_scale;
    int s = 2;
    int bh, bw;
    GetDrawTextBoxSize(text, 500, &bh, &bw);
    Rectangle area = {x, y, ui_scale * (bw + 6), ui_scale * (bh + 6)};
    if (y > GetScreenHeight() / 2) {
      area.y = area.y - area.height - 16 - 8;
      y = area.y;
    }
    if (x > GetScreenWidth() / 2) {
      area.x = area.x - area.width - 16;
      x = area.x;
    }
    DrawRectangle(area.x - s, area.y - s, area.width + 2 * s, area.height + 2 * s, BLACK);
    DrawRectangleRec(area, kk);
    rlPushMatrix();
    rlScalef(ui_scale, ui_scale, 1);
    y = y / ui_scale + 4;
    x = x / ui_scale + 4;
    DrawTextBox(text, (Rectangle){x, y, 500, 0}, fc, NULL);
    rlPopMatrix();
  }
}

bool BtnHover(Btn* b)
{
  if (b->disabled || b->hidden) return false;
  Vector2 pos = GetMousePosition();
  bool hit = CheckCollisionPointRec(pos, b->hitbox);
  return hit;
}

void BtnDrawColor(Ui* ui, Rectangle r, Color c, bool selected, bool disabled)
{
  int s = ui->scale;
  int x = r.x + 1 * s;
  int y = r.y + 1 * s;
  int w = r.width - 2 * s;
  int h = r.height - 2 * s;
  Color c0 = GetLutColor(COLOR_BTN0);
  Color c1 = GetLutColor(COLOR_BTN1);
  Color c2 = GetLutColor(COLOR_BTN2);
  Color cbg = GetLutColor(COLOR_BTN_BG);
  if (!disabled && selected) {
    DrawRectangle(x - s, y - s, w + 2 * s, h + 2 * s, cbg);
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
  }
  else {
    DrawRectangle(x + 2 * s, y + 2 * s, w - 4 * s, h - 4 * s, c);
  }
}
