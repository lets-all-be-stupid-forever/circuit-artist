#ifndef WIDGETS_H
#define WIDGETS_H
#include "common.h"

// Scroll widget.
typedef struct {
  Rectangle content;
  Rectangle rect;
  Rectangle scroll_rect;
  int value;  /* Actual scroll value */
  int value0; /* Value of scroll when user has pressed down the button*/
  int mouse0; /* mouse y position when user pressed down button */
  bool down;
  bool hit;
  bool hidden;
} Scroll;

// Scroll Functions
void scroll_init(Scroll* s);
void scroll_set_content_box(Scroll* s, Rectangle box, int w);
void scroll_update(Scroll* s, float h, v2 mouse);
void scroll_scroll_to_bottom(Scroll* s, float h);
void scroll_draw(Scroll* s);
void scroll_reset_value(Scroll* s);
int scroll_get_value(Scroll* s);

// Listbox row widget, used internally in the Listbox widget.
typedef struct {
  char* content;
  Rectangle hitbox_r;  // relative hitbox
  Rectangle hitbox_g;
  int text_y;
  sprite_t sprite;
} ListboxRow;

// Listbox widget.
typedef struct {
  ListboxRow* rows;
  Rectangle hitbox;
  int height;
  int row_hit;
  int row_pad;
  Scroll scroll;
} Listbox;

// Listbox
void listbox_init(Listbox* l);
void listbox_destroy(Listbox* l);
void listbox_set_box(Listbox* l, Rectangle r);
void listbox_add_row(Listbox* l, const char* content);
void listbox_add_row_icon(Listbox* l, const char* content, sprite_t sprite);
void listbox_clear(Listbox* l);
void listbox_update(Listbox* l);
void listbox_draw(Listbox* l, int sel);

// Textbox widget. Renders text using the DrawTextBoxAdvanced routine.
typedef struct {
  Rectangle box;
  char* text;
  sprite_t* sprites;
  int text_w;
  int text_x;
  int height;
  Color bg;
  Scroll scroll;
} Textbox;

// Text Box widget functions
void textbox_init(Textbox* t);
void textbox_destroy(Textbox* t);
void textbox_set_bg(Textbox* t, Color bg);
void textbox_set_content(Textbox* t, const char* txt, sprite_t* sprites);
void textbox_set_box(Textbox* t, Rectangle box);
void textbox_update(Textbox* t);
void textbox_draw(Textbox* t);
void textbox_scroll_to_bottom(Textbox* t);
void textbox_calc_height(Textbox* t);

// Button widget.
// The caller has to setup its properties manually at creation.
typedef struct {
  bool disabled;
  bool pressed;
  bool toggled;
  bool hover;
  bool hidden;
  bool gradient;
  Rectangle hitbox;
} Btn;

// Button
bool btn_update(Btn* b);
void btn_draw_text(Btn* b, int ui_scale, const char* text);
void btn_draw_text_primary(Btn* b, int ui_scale, const char* text);
void btn_draw_legend(Btn* b, int ui_scale, const char* text);
void btn_draw_icon(Btn* b, int ui_scale, Texture2D tex, Rectangle r);
void btn_draw_color(Rectangle r, Color c, bool selected, bool disabled);
bool btn_hover(Btn* b);

typedef struct {
  int maxValue;
  int minValue;
  int value;
  int cursorValue;
  bool pressed;
  bool released;
  Rectangle hitbox;
} Slider;

bool slider_update(Slider* slider);
void slider_draw(Slider* slider);

void draw_widget_frame(Rectangle inner_content);
void draw_widget_frame_inv(Rectangle r);

typedef struct {
  Rectangle hitbox;
  char txt[256];
  int tlen;
  int cursor_pos;  // Cursor position in text
  bool active;
  float alive;
} Editbox;

void editbox_init(Editbox* b);
bool editbox_update(Editbox* b);
void editbox_draw(Editbox* b);
bool rect_hover(Rectangle hitbox, Vector2 pos);

typedef struct {
  Rectangle hitbox;
  char txt[256];
} Label;

void label_set_text(Label* l, const char* txt);
void label_draw(Label* l);

#endif
