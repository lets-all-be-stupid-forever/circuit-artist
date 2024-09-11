#ifndef LISTBOX_H
#define LISTBOX_H
#include "defs.h"
#include "img.h"
#include "sprite.h"
#include "ui.h"

// Scroll widget.
typedef struct {
  Rectangle content;
  Rectangle rect;
  Rectangle scroll_rect;
  int value;
  int value0;
  int mouse0;
  bool down;
  bool hit;
  bool hidden;
} Scroll;

// Listbox row widget, used internally in the Listbox widget.
typedef struct {
  char* content;
  Rectangle hitbox_r;  // relative hitbox
  Rectangle hitbox_g;
  int text_y;
} ListboxRow;

// Listbox widget.
typedef struct {
  ListboxRow* rows;
  Rectangle hitbox;
  int height;
  int row_hit;
  Scroll scroll;
} Listbox;

// Textbox widget. Renders text using the DrawTextBoxAdvanced routine.
typedef struct {
  Rectangle box;
  char* text;
  Sprite* sprites;
  int text_w;
  int text_x;
  int height;
  Scroll scroll;
} Textbox;

// Button widget.
// The caller has to setup its properties manually at creation.
typedef struct {
  bool disabled;
  bool pressed;
  bool toggled;
  bool hover;
  bool hidden;
  Rectangle hitbox;
} Btn;

// Listbox
void ListboxLoad(Listbox* l);
void ListboxUnload(Listbox* l);
void ListboxSetBox(Listbox* l, Rectangle r);
void ListboxAddRow(Listbox* l, const char* content);
void ListboxUpdate(Listbox* l);
void ListboxDraw(Listbox* l, Ui* ui, int sel);

// Scroll Functions
void ScrollLoad(Scroll* s);
void ScrollSetContentBox(Scroll* s, Rectangle box, int w);
void ScrollUpdate(Scroll* s, float h, Vector2 mouse);
void ScrollDraw(Scroll* s);
void ScrollResetValue(Scroll* s);
int ScrollGetValue(Scroll* s);

// Text Box widget functions
void TextboxLoad(Textbox* t);
void TextboxUnload(Textbox* t);
void TextboxSetContent(Textbox* t, const char* txt, Sprite* sprites);
void TextboxSetBox(Textbox* t, Rectangle box);
void TextboxUpdate(Textbox* t, Ui* ui);
void TextboxDraw(Textbox* t, Ui* ui);

// Updates button state. Returns true if button was clicked.
bool BtnUpdate(Btn* b, Ui* ui);

// Text button (close button for example).
void BtnDrawText(Btn* b, int ui_scale, const char* text);

// Draws the legend of the button. Needs to be called separately from the button itself.
void BtnDrawLegend(Btn* b, int ui_scale, const char* text);

// Icon button: tool buttons and challenge button.
void BtnDrawIcon(Btn* b, int ui_scale, Texture2D tex, Rectangle r);

// Colorbox button (displayed in the bottom of the UI.
void BtnDrawColor(Ui* ui, Rectangle r, Color c, bool selected, bool disabled);

// Checks if button is being hovered
bool BtnHover(Btn* b);

#endif
