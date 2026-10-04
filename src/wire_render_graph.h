#ifndef CA_WIRE_BLOCK_H
#define CA_WIRE_BLOCK_H
#include "stb_ds.h"
#include "tex.h"

typedef struct {
  int off;
  int size;
} WireBlock;

typedef struct {
  int nwire;
  int num_blocks;
  Vector4* pos;
  Vector2* dist;
  int* wids;
  WireBlock* wire_block;
} WireRenderGraph;

void wrg_init(WireRenderGraph* r, int nwire);
void wrg_destroy(WireRenderGraph* r);

static inline void wrg_count_wire(WireRenderGraph* r, int wid) {
  int bid = wid / 32;
  if (r->wire_block[bid].off == -1) {
    if (bid == 0) {
      r->wire_block[bid].off = 0;
      r->wire_block[bid].size = 0;
    } else {
      r->wire_block[bid].off =
          r->wire_block[bid - 1].off + r->wire_block[bid - 1].size;
      r->wire_block[bid].size = 0;
    }
  }
  r->wire_block[bid].size++;
}

/* rc_seg is deduced from layer */
static inline void wrg_addvseg(WireRenderGraph* r, int wid, int l, int x,
                               int y0, int y1, float rc_seg, float d0,
                               float d1) {
  Vector4 pos = {x, y0, y1, rc_seg};
  Vector2 dist = {d0, d1};
  arrput(r->pos, pos);
  arrput(r->dist, dist);
  /* Encodes the wire id (last 28 bits), then layer (3 next bits) then
   * direction(last bit)*/
  int wid_gpu = ((wid + 0) << 4) | (l << 1) | 1;
  arrput(r->wids, wid_gpu);
  wrg_count_wire(r, wid);
}

static inline void wrg_addhseg(WireRenderGraph* r, int wid, int l, int x0,
                               int x1, int y, float rc_seg, float d0,
                               float d1) {
  Vector4 pos = {y, x0, x1, rc_seg};
  Vector2 dist = {d0, d1};
  // printf("d=%f %f\n", d0, d1);
  int wid_gpu = ((wid + 0) << 4) | (l << 1) | 0;
  arrput(r->pos, pos);
  arrput(r->dist, dist);
  arrput(r->wids, wid_gpu);
  wrg_count_wire(r, wid);
}

#endif
