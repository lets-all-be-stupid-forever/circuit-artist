#include "pixel_graph.h"

#include "img.h"
#include "nand_detection.h"
#include "profiler.h"
#include "stb_ds.h"

#define FLAG_LVIA (1 << 5)
#define FLAG_DRV (1 << 6)

// Single pixels
//    U
//  L M R
//    B
#define bM (1 << 0)
#define bU (1 << 1)
#define bR (1 << 2)
#define bB (1 << 3)
#define bL (1 << 4)

// L turns
// Example:
// bL1 =
//    1
//  0 1 1
//    0
#define bL1 (bM | bU | bR)
#define bL2 (bM | bU | bL)
#define bL3 (bM | bB | bR)
#define bL4 (bM | bB | bL)

// Corners
#define bC1 ((bM | bB))
#define bC2 ((bM | bU))
#define bC3 ((bM | bL))
#define bC4 ((bM | bR))

// T's
#define bT1 ((bM | bR | bL | bU))
#define bT2 ((bM | bR | bL | bB))
#define bT3 ((bM | bB | bU | bR))
#define bT4 ((bM | bB | bU | bL))

/* All */
#define bA ((bM | bB | bU | bL | bR))

#define bV (bM | bU | bB)

/* Neighbourhood code:
 * bit1: is wether the pixel is a fg or bg.
 * bit2-5: foreground flag for each of 4-neighbour
 * bit6: flag if pixel has a "via" to the upper layer.
 * bit7: flag id pixel is
 */
static void sim_parse_code(int w, int h, u8* code, u8* code_up) {
  int wc = w;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      int b0 = code[idx] & 1;
      int b1 = y > 0 ? (code[idx - w] & 1) : 0;     /* up */
      int b2 = x < w - 1 ? (code[idx + 1] & 1) : 0; /* right */
      int b3 = y < h - 1 ? (code[idx + w] & 1) : 0; /* bot */
      int b4 = x > 0 ? (code[idx - 1] & 1) : 0;     /* left */
      u8 b = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3) | (b4 << 4);
      if (code_up) {
        u8 c = code_up[(y)*wc + (x)] & 0x1F;
        u8 via =
            ((c == bC1) || (c == bC2) || (c == bC3) || (c == bC4) || (c == bM));
        b |= (via << 5);
      }
      code[idx] |= b;
    }
  }
}

/*
 * Builds the pixel graph.
 * Each pixel has 2 coordinates:
 *
 * idx (positive)
 *   Used in horizontal-segment node.
 * -idx-1 (negative)
 *   Used in vertical-segment node.
 *
 * This way, pixels in cross-sections will have a node for the horizontal
 * segment and one for the vertical direction.
 *
 * For multi-layer, we use a index system with stride w0*h0.
 * This way, we have a single index system for everything.
 * From the graph we can infer connected wires components and distances.
 *
 * Types of edges:
 * - Horizontal edges: We connect right to left.
 * - Vertical edges: We connect bottom to top.
 * - Horizontal-Vertical edges: We connect at each T/L intersection.
 * - Layer Edge: We connect from bottom layer to upper layer.
 *
 * Layer edge connects to both H and V nodes (but on top is either H or V).
 * */
static Graph build_graph_from_code(int nl, int w, int h, u8** img_code) {
  Graph _g = {0};
  Graph* g = &_g;
  graph_init(g, 20);
  int w0 = w;
  int h0 = h;
  typedef struct {
    int n;
    int i;
  } NodeTrack;
  /* Pixel "length" per layer.
   * The idea is to make paths "longer" in lower layers, to simulate highest
   * resistance / smaller wires, and make the upper layers propagate
   * information faster.
   */
  NodeTrack* pv = calloc(w0, sizeof(NodeTrack));
  for (int l = nl - 1; l >= 0; l--) {
    int wu = w;
    int hu = h;

    /* int d_via = 5 * pl;  */
    /* UPDATED: Using 0 as distance across hseg/vseg vias so we dont punish
     * diagonals */
    float d_via = 0;

    /* Moving between layers cost 1 pixel of the layer below
     * Layer vias are flagged on the bottom layer.
     * */
    float d_layer = 1;

    u8* code = img_code[l];
    NodeTrack ph = {-1, -1};
    for (int x = 0; x < w; x++) {
      pv[x] = (NodeTrack){-1, -1};
    }

#define V_EDGE graph_add_edge(g, pv[x].n, nv, (-(iv - pv[x].i) / w))
#define H_EDGE graph_add_edge(g, ph.n, nh, (ih - ph.i))

    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int nv = -1;
        int nh = -1;
        int idx = y * w + x;
        /* h-seg node */
        int ih = idx + l * w0 * h0;
        /* v-seg node */
        int iv = -ih - 1;
        int c = code[idx];
        int b = c & 31;
        int nb = ~b;
        /* Background pixel */
        if ((~b) & bM) {
          continue;
        }

        /* 0 - h - 1 */
        if ((nb & bL) && (b & bR)) {
          assert(ph.n == -1);
          nh = graph_add_node(g, ih);
          ph = (NodeTrack){nh, ih};
        }

        /* 1 - h - 0 */
        if ((b & bL) && (nb & bR)) {
          assert(ph.n != -1);
          nh = graph_add_node(g, ih);
          H_EDGE;
          ph = (NodeTrack){-1, -1};
        }

        /* 0 \ v \ 1*/
        if ((nb & bU) && (b & bB)) {
          assert(pv[x].n == -1);
          nv = graph_add_node(g, iv);
          pv[x] = (NodeTrack){nv, iv};
        }

        /* 1 \ v \ 0 */
        if ((b & bU) && (nb & bB)) {
          assert(pv[x].n != -1);
          nv = graph_add_node(g, iv);
          /* Vertical edge */
          V_EDGE;
          pv[x] = (NodeTrack){-1, -1};
        }

        /* Single pixel */
        if (b == bM) {
          assert(pv[x].n == -1);
          assert(ph.n == -1);
          nh = graph_add_node(g, ih);
          // graph_add_edge(g, nh, nh, 0); /* d=0 */
        }

        /* nh!=-1, nv=1 implies a T or a corner.
         * the check with the upper pixel eliminates the corner case
         *
         * nh >0 means there's a new horizontal pixel
         * nv == -1 and b&bU means the V seg is straight (can't be a lone
         * pixel).
         *
         * */
        if (nh != -1 && nv == -1 && (b & bU)) {
          /* A T
           *     |
           * ----|
           *     |
           *     OR
           *     |
           *     |-----
           *     |
           * */
          assert(pv[x].n != -1);
          nv = graph_add_node(g, iv);
          /* Connect the new v node with previous V */
          V_EDGE;
          pv[x] = (NodeTrack){nv, iv};
          /* Connect the new v node with the H node */
          graph_add_edge(g, nv, nh, d_via); /* d= D_VIA */
        } else if (nv != -1 && nh == -1 && (b & bL)) {
          /* A T */
          nh = graph_add_node(g, ih);
          H_EDGE;
          ph = (NodeTrack){nh, ih};
          graph_add_edge(g, nv, nh, d_via); /* d= D_VIA */
        } else if (nv != -1 && nh != -1) {
          /* An L */
          graph_add_edge(g, nv, nh, d_via); /* d=D_VIA*/
        }

        /* Checks if it's a driver */
        if (c & FLAG_DRV) {
          if ((nv == -1) && (nh == -1)) {
            if (ph.n != -1) {
              nh = graph_add_node(g, ih);
              H_EDGE;
              ph = (NodeTrack){nh, ih};
            } else {
              assert(pv[x].n != -1);
              nv = graph_add_node(g, iv);
              V_EDGE;
              pv[x] = (NodeTrack){nv, iv};
            }
          }
        }

        if (c & (FLAG_LVIA)) {
          if ((pv[x].n != -1) && (nv == -1)) {
            nv = graph_add_node(g, iv);
            V_EDGE;
            pv[x] = (NodeTrack){nv, iv};
          }
          if ((ph.n != -1) && (nh == -1)) {
            nh = graph_add_node(g, ih);
            H_EDGE;
            ph = (NodeTrack){nh, ih};
          }
#if 0
          int xu = x/2;
          int yu = y/2;
#else
          int xu = x;
          int yu = y;
#endif
          int ihu = (l + 1) * (w0 * h0) + (yu * wu + xu);
          int ivu = -ihu - 1;
          int nvu = -1;
          int nhu = -1;
          bool ok1 = graph_node_inv(g, ihu, &nhu);
          bool ok2 = graph_node_inv(g, ivu, &nvu);
          assert(ok1 || ok2);

          if (nh != -1) assert(g->nodes[nh] == ih);
          if (nv != -1) assert(g->nodes[nv] == iv);

#if 1
          if (nv != -1 && nvu != -1) graph_add_edge(g, nv, nvu, d_layer);
          if (nv != -1 && nhu != -1) graph_add_edge(g, nv, nhu, d_layer);
          if (nh != -1 && nvu != -1) graph_add_edge(g, nh, nvu, d_layer);
          if (nh != -1 && nhu != -1) graph_add_edge(g, nh, nhu, d_layer);
#endif
        }
      }
    }
  }
  free(pv);
  return *g;
}

/* Separates background from foreground in a regular image. */
static u8* gen_fg_code(Image img) {
  int w = img.width;
  int h = img.height;
  u8* image = calloc(w * h, sizeof(u8));
  assert(img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  Color* colors = img.data;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      image[y * w + x] = colors[y * w + x].a != 0;
    }
  }
  return image;
}

static void debug_imgcode(int nl, int w, int h, u8** img_code) {
  for (int l = 0; l < nl; l++) {
    Image tmp = GenImageColor(w, h, BLACK);
    Color* pix = tmp.data;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int idx = y * w + x;
        pix[idx] = (Color){
            img_code[l][idx],
            img_code[l][idx],
            img_code[l][idx],
            255,
        };
      }
    }
    char fname[100];
    snprintf(fname, sizeof(fname), "l%d.png", l);
    ExportImage(tmp, fname);
    printf("Saved %s.\n", fname);
  }
}

static void draw_line2(int w, int x0, int y0, int x1, int y1, Color* pix) {
  if (x0 == x1) {
    if (y0 > y1) {
      int tmp = y1;
      y1 = y0;
      y0 = tmp;
    }
    pix[y0 * w + x0].r = 255;
    for (int y = y0 + 1; y <= y1 - 1; y++) {
      Color q = pix[y * w + x0];
      pix[y * w + x0].r = 128;
    }
    pix[y1 * w + x1].r = 255;
  } else {
    if (x0 > x1) {
      int tmp = x1;
      x1 = x0;
      x0 = tmp;
    }
    pix[y0 * w + x0].g = 255;
    for (int x = x0 + 1; x <= x1 - 1; x++) {
      Color q = pix[y0 * w + x];
      pix[y0 * w + x].g = 128;
    }
    pix[y1 * w + x1].g = 255;
  }
}

static void debug_edge_graph(int l, int w0, int h0, Graph* g) {
  int se = g->max_edges;
  int s = w0 * h0;
  // int w1 = w0 / 2;
  // int h1 = h0 / 2;
  int ww = w0;  // l == 0 ? w0 : w1;
  int hh = h0;  // l == 0 ? h0 : h1;
  Image tmp = GenImageColor(ww, hh, BLACK);
  Color* pix = tmp.data;
  for (int i = 0; i < g->n; i++) {
    int l0, x0, y0, x1, y1, l1;
    int idx0 = g->nodes[i];
    find_idx(s, w0, idx0, &l0, &y0, &x0);
    int ne = g->ecount[i];
    for (int j = 0; j < ne; j++) {
      int i1 = g->edges[i * se + j].e;
      /* printf("E: %d %d\n", i, i1); */
      int idx1 = g->nodes[i1];
      find_idx(s, w0, idx1, &l1, &y1, &x1);
      if (l0 == l1 && l0 == l) {
        draw_line2(ww, x0, y0, x1, y1, pix);
      } else {
        if (l0 > l1) {
          assert(y0 == y1);
          assert(x0 == x1);
        } else if (l1 < l0) {
          assert(y1 == y0);
          assert(x1 == x0);
        }
      }
    }
  }
  char fname[50];
  snprintf(fname, sizeof(fname), "g%d.png", l);
  ExportImage(tmp, fname);
  printf("Created %s.\n", fname);
}

static void debug_edges(int nl, int w, int h, Graph* g) {
  for (int l = 0; l < nl; l++) {
    debug_edge_graph(l, w, h, g);
  }
}

static void pixel_graph_register_external_pins(PixelGraph* pg, int w, int h,
                                               PinGroup* p) {
  int np = arrlen(p);
  for (int ip = 0; ip < np; ip++) {
    int nw = arrlen(p[ip].pins);
    int t = p[ip].type;
    if (t == PIN_IMG2LUA) {
      arrput(pg->pgoff, arrlen(pg->skt));
    } else {
      arrput(pg->pgoff, arrlen(pg->drv));
    }
    for (int iw = 0; iw < nw; iw++) {
      int y = p[ip].pins[iw].y;
      int x = p[ip].pins[iw].x;
      if (y < 0) y += h;
      if (x < 0) x += w;

      int idx = y * w + x;
      /* Invalid wire indexes (TODO: add some sort of warning?) */
      if (y < 0 || y >= h || x < 0 || x >= w) {
        idx = -1;
      }
      if (t == PIN_IMG2LUA) {
        arrput(pg->skt, idx);
      } else {
        arrput(pg->drv, idx);
      }
    }
  }
}

static inline int get_crossing_pixel_direction(int* pixels, int w, int h, int x,
                                               int y) {
  if (x == 0 || x == w - 1 || y == 0 || y == h - 1) {
    return false;
  }
  int idx = y * w + x;
  int c = pixels[idx];
  int nh = 0;
  int nv = 0;
  nv += pixels[idx - w] == c;
  nv += pixels[idx + w] == c;
  nh += pixels[idx - 1] == c;
  nh += pixels[idx + 1] == c;
  if (nv > nh) {
    return 1;
  } else {
    return 0;
  }
}

void find_pixel_orientation(int w, int h, int* pixels, u8* code, u8* ori) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      u8 pc = code[idx] & (bA);
      int v = 0;
      if (pc == bA) {
        v = get_crossing_pixel_direction(pixels, w, h, x, y);
      } else if (pc == bV || (pc == (bM | bU)) || (pc == (bM | bB))) {
        v = 1;
      } else {
        v = 0;
      }
      ori[idx] = v;
    }
  }
}

void pixel_graph_init(PixelGraph* pg, DistSpec spec, int nl, Image* imgs,
                      PinGroup* p, bool debug) {
  profiler_tic_single("pixel_graph");
  miniprof_reset();
  miniprof_time();
  int w = imgs[0].width;
  int h = imgs[0].height;
  u8* img_code[MAX_LAYERS + 1] = {0};
  for (int i = 0; i < nl; i++) {
    img_code[i] = gen_fg_code(imgs[i]);
  }

  find_nands(w, h, img_code[0], &pg->nands);
  remove_nand_pixels(w, img_code[0], pg->nands);

  /* Nand sockets/drivers */
  int nn = arrlen(pg->nands);
  for (int i = 0; i < nn; i++) {
    int drv = pg->nands[i].d;
    arrput(pg->skt, pg->nands[i].s1);
    arrput(pg->skt, pg->nands[i].s2);
    arrput(pg->drv, drv);
  }
  pixel_graph_register_external_pins(pg, w, h, p);
  miniprof_time();  // T0 ends

  /* Sockets and Drivers always have dedicated nodes */
  for (int i = 0; i < arrlen(pg->drv); i++) {
    img_code[0][pg->drv[i]] |= FLAG_DRV;
  }
  for (int i = 0; i < arrlen(pg->skt); i++) {
    img_code[0][pg->skt[i]] |= FLAG_DRV;
  }

  miniprof_time();  // T1 ends
  /* This scales poorly with layers. Can be done on GPU ! */
  for (int l = nl - 1; l >= 0; l--) {
    sim_parse_code(w, h, img_code[l], img_code[l + 1]);
    int wl = imgs[l].width;
    int hl = imgs[l].height;
    pg->ori[l] = calloc(wl * hl, sizeof(u8));
    find_pixel_orientation(wl, hl, imgs[l].data, img_code[l], pg->ori[l]);
  }
  miniprof_time();  // T2 ends
  /* This is the slowest part */
  pg->g = build_graph_from_code(nl, w, h, img_code);
  miniprof_time();  // T3 ends
  if (debug) {
    debug_imgcode(nl, w, h, img_code);
  }
  if (debug) {
    debug_edges(nl, w, h, &pg->g);
  }
  /* shutdown */
  for (int l = 0; l < nl; l++) {
    free(img_code[l]);
  }
  miniprof_time();  // T4 ends
  profiler_tac_single("pixel_graph");

  // save_img_u8(w, h, pg->ori[0], "../ori.png");
  miniprof_nxt();
  miniprof_print("pixel_graph");
  // printf("Pixel Graph:\n");
  // graph_print_weights(&pg->g);
}

void pixel_graph_destroy(PixelGraph* pg) {
  for (int i = 0; i < MAX_LAYERS; i++) {
    free(pg->ori[i]);
  }
  arrfree(pg->drv);
  arrfree(pg->skt);
  arrfree(pg->nands);
  arrfree(pg->pgoff);
  graph_destroy(&pg->g);
}
