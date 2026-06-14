#ifndef CA_LAYOUT_H
#define CA_LAYOUT_H
#include "raylib.h"
#include "widgets.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
  struct {
    char* key;
    Rectangle value;
  }* dict;  // id -> node
  Vector2 off;
  bool align_left;
  bool align_top;
} Layout;

Layout* parse_layout(const char* fname);
void layout_update_offset(Layout* l);
void layout_update_offset_region(Layout* l, Rectangle region);
Rectangle layout_rect(Layout* l, const char* id);
Rectangle layout_rectb(Layout* l, const char* id);
void layout_free(Layout* l);
Layout* easy_load_layout(const char* win_name);
void layout_scale(Layout* l);

#if defined(__cplusplus)
}
#endif

#endif
