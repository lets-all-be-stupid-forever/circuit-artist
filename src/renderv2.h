#ifndef CA_RENDERV2_H
#define CA_RENDERV2_H
#include "stb_ds.h"
#include "tex.h"

typedef struct {
  int idx;
  int rot;
  Color c1; /* Socket 1 */
  Color c2; /* Socket 2 */
  Color c3; /* Driver */
} NandDesc;

typedef struct {
  int activation_counter;
  int id_nand;
  int next_value;
  int max_delay;
  float de;  // energy spent per tick
} NandState;

typedef struct {
  int off;
  int size;
} WireBlock;

typedef struct {
  int w;          /* Circuit width */
  int h;          /* Circuit height */
  int nwire;      /* Total wires (not segments) */
  int tickmod;    /* Tick mod for visualization */
  int tickgap;    /* Tick gap for visualization */
  int error_mode; /* Error mode flag */
  /* Dirty map optimization */
  int num_blocks;        /* Num wire blocks */
  WireBlock* wire_block; /* Block for dirty flags */
  int* wids;             /* Wire IDs (gpu version) */
  Vector2* dist;         /* Distances of each segment end */
  Vector4* pos;          /* Position of segments */
  /* NAND stuff */
  Vector2* nand_pos; /* Position of the nands (3 pixels each) */
  Vector4* nand_clr; /* Normalized color (3 colors each)*/
  int nand_vao;      /* VAO for NAND programs */
  u32 nand_vbo_pos;  /* VBO for NAND pixels */
  u32 nand_vbo_clr;  /* VBO for NAND colors */
  u32 nand_vbo_vert; /* VBO for vertices*/
  /* NAND Activation stuff */
  Vector4* nandact_phase;
  Vector2* nandact_pos;
  Vector4* nandact_cnand1;
  Vector4* nandact_cnand2;
  Vector4* nandact_cnand3;
  int nandact_vao;        /* VAO for nandact programs */
  u32 nandact_vbo_pos;    /* VBO for nandact pixels */
  u32 nandact_vbo_vert;   /* VBO for nandact vertices */
  u32 nandact_vbo_cnand1; /* VBO for nandact color1 */
  u32 nandact_vbo_cnand2; /* VBO for color1 */
  u32 nandact_vbo_cnand3; /* VBO for color3 */
  u32 nandact_vbo_phase;  /* VBO for phase */
  int* bad_nand_ids;      /* IDs of bad/error NANDs for error mode */
  /* NAND error shader (simplified) */
  Vector2* nandacterr_pos;
  float* nandacterr_rot;
  int nandacterr_vao;
  u32 nandacterr_vbo_pos;
  u32 nandacterr_vbo_rot;
  u32 nandacterr_vbo_vert;
  /* Error pixels */
  Vector2* err_pos;     /* Coordinates with error */
  int err_vao;          /* VAO for pixel error */
  u32 err_vbo_pos;      /* VBO for coordinates */
  u32 err_vbo_vertices; /* VBO for vertices  */
  /* Pmap VBO's */
  Tex* pmap;                        /* Rendered PMAP (contains pulse data) */
  int vao;                          /* VAO for Pmap */
  u32 vbo_wids;                     /* VBO for wid */
  u32 vbo_pos;                      /* VBO for segment position */
  u32 vbo_vertices;                 /* VBO for vertices */
  u32 vbo_dist;                     /* VBO for node distances */
  int nl;                           /* Num layers */
  RenderTexture layers[MAX_LAYERS]; /* Borrowed layer textures */
  Tex* combined_layers; /* Layer that is actually rendered (combines) */
  int hide_mask;        /* Bit0 = visibility of layer 0 */
  Tex* acc_c;           /* Accumulated (EMA) circuit texture */
  Tex* acc_l;           /* Accumulated (EMA) light texture */
  bool full_pmap_update;
  Color bg_color; /* Color outside the circuit */
} RenderV2;

RenderV2* renderv2_create(int w, int h, int nwire, int nl,
                          RenderTexture* layers);
void renderv2_update_hidden_mask(RenderV2* r, int hide_mask);
void renderv2_prepare(RenderV2* r, int tickmod, int tickgap);
void renderv2_update_pulse(RenderV2* r, Texture pulses, uint32_t* dirty_mask);
void renderv2_addnand(RenderV2* r, Vector2 p0, Vector2 p1, Vector2 p2, Color c0,
                      Color c1, Color c2);
Tex* renderv2_render(RenderV2* r, Cam2D cam, int tw, int th, int ns,
                     float frame_steps, NandDesc* nidx, NandState* states,
                     bool use_neon, Times times);
void renderv2_free(RenderV2* r);
void renderv2_add_err_pixel(RenderV2* r, int x, int y);
void renderv2_add_bad_nand(RenderV2* r, int nand_id);

static inline void renderv2_count_wire(RenderV2* r, int wid) {
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
static inline void renderv2_addvseg(RenderV2* r, int wid, int l, int x, int y0,
                                    int y1, float rc_seg, float d0, float d1) {
  Vector4 pos = {x, y0, y1, rc_seg};
  Vector2 dist = {d0, d1};
  arrput(r->pos, pos);
  arrput(r->dist, dist);
  /* Encodes the wire id (last 28 bits), then layer (3 next bits) then
   * direction(last bit)*/
  int wid_gpu = ((wid + 0) << 4) | (l << 1) | 1;
  arrput(r->wids, wid_gpu);
  renderv2_count_wire(r, wid);
}

static inline void renderv2_addhseg(RenderV2* r, int wid, int l, int x0, int x1,
                                    int y, float rc_seg, float d0, float d1) {
  Vector4 pos = {y, x0, x1, rc_seg};
  Vector2 dist = {d0, d1};
  // printf("d=%f %f\n", d0, d1);
  int wid_gpu = ((wid + 0) << 4) | (l << 1) | 0;
  arrput(r->pos, pos);
  arrput(r->dist, dist);
  arrput(r->wids, wid_gpu);
  renderv2_count_wire(r, wid);
}

#endif
