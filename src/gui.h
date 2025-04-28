#ifndef GUI_H
#define GUI_H

struct widget;

typedef struct widget {
  int id;
  int x;
  int y;
  int w;
  int h;
  struct widget* parent;
  struct widget** children;
} widget;

widget* gui_container();
widget* gui_btn();
void gui_add(widget* p, widget* c);
void gui_del(widget* w);
void gui_layout(widget* w);

#endif
