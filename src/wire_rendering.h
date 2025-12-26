#ifndef CA_WIRE_RENDERING_H
#define CA_WIRE_RENDERING_H
#include "common.h"
#include "dist_graph.h"
#include "img.h"
#include "raylib.h"
#include "tex.h"

// This rendered uses a 2D mesh-based wire renderedr
//
// Idea: the image is composed by a list of 2D segments. Each segment is just a
// rectangle: horizontal or vertical.
//
typedef struct {
  int w;
  int h;
  Texture2D dmap;
  // RenderTexture2D pmap;
  Tex* pmap;
  RenderTexture2D tgt;        /* not necessary */
  RenderTexture2D tgt_bright; /* not necessary */
  bool prepared;
  int numinst;
  int ssbo0;
  int vao;

  int num_wires;

  u32 vbo_wids;
  u32 vbo_pos;
  u32 vbo_vertices;

  Camera3D camera;
  int error_mode;
} WireRenderer;

void wire_renderer_init(WireRenderer* v, int w, int h);
void wire_renderer_set_error_mode(WireRenderer* v, int error_mode);
void wire_renderer_set_dmap(WireRenderer* v, float* dmap);
void wire_renderer_prepare(WireRenderer* v, int num_wires, int num_segs,
                           WireSegment* seglist);
void wire_renderer_update_pmap(WireRenderer* v, Texture pulses);
void wire_renderer_destroy(WireRenderer* v);

#endif
