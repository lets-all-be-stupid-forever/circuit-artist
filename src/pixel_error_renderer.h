#ifndef CA_PIXEL_ERROR_H
#define CA_PIXEL_ERROR_H
#include "common.h"

typedef struct {
  int vao;
  int* pos;
  int vbo;
  u32 vbo_vertices;
} pixel_error_renderer_t;

void pixel_error_renderer_init(pixel_error_renderer_t* r);
void pixel_error_renderer_add_pixel(pixel_error_renderer_t* r, int x, int y);
void pixel_error_renderer_prepare(pixel_error_renderer_t* r);
void pixel_error_renderer_render(pixel_error_renderer_t* r, Cam2D cam,
                                 float utime);
void pixel_error_renderer_destroy(pixel_error_renderer_t* r);

#endif
