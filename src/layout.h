#ifndef CA_LAYOUT_H
#define CA_LAYOUT_H
#include "raylib.h"
#include "widgets.h"

typedef struct {
  struct {
    char* key;
    Rectangle value;
  }* dict;  // id -> node
  Vector2 off;
} Layout;

Layout* parse_layout(const char* fname);
void layout_update_offset(Layout* l);
Rectangle layout_rect(Layout* l, const char* id);
void layout_free(Layout* l);
Layout* easy_load_layout(const char* win_name);

#endif
