#ifndef CA_BRUSH_H
#define CA_BRUSH_H
#include "common.h"

// Context for 2d brush_t tool.
//
// A brush context is basically a concatenated list of 2D points that is
// created while the user drags his mouse with cursor down.
typedef struct {
  int path_cnt;       // Number of points in the current path.
  int max_path_size;  // pixel path capacity
  int* path_pixels;   // XY pixel points X0,Y0,X1,Y1,X2,Y1, ...
} brush_t;

void brush_init(brush_t* b);
void brush_destroy(brush_t* b);
bool brush_append_point(brush_t* b, int px, int py);
void brush_reset(brush_t* b);
void brush_make_image(brush_t* b, Color c, int w, int h, Image* out, v2i* off);

#endif
