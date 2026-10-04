#include "wire_render_graph.h"

void wrg_init(WireRenderGraph* r, int nwire) {
  *r = (WireRenderGraph){0};
  r->nwire = nwire;
  r->num_blocks = (r->nwire + 31) / 32;
  r->wire_block = calloc(r->num_blocks, sizeof(WireBlock));
  for (int i = 0; i < r->num_blocks; i++) {
    r->wire_block[i] = (WireBlock){-1, -1};
  }
}

static inline Vector4 clr2vec4(Color c) {
  return (Vector4){
      c.r / 255.0f,
      c.g / 255.0f,
      c.b / 255.0f,
      c.a / 255.0f,
  };
}

void wrg_destroy(WireRenderGraph* r) {
  free(r->wire_block);
  arrfree(r->pos);
  arrfree(r->wids);
  arrfree(r->dist);
}

