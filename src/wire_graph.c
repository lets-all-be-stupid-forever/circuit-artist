#include "wire_graph.h"

#include "connected_components.h"
#include "profiler.h"
#include "stb_ds.h"
#include "stdlib.h"

static void draw_line(int w, int x0, int y0, int x1, int y1, Color* pix,
                      Color c) {
  if (x0 == x1) {
    if (y0 > y1) {
      int tmp = y1;
      y1 = y0;
      y0 = tmp;
    }
    for (int y = y0; y <= y1; y++) {
      pix[y * w + x0] = c;
    }
  } else {
    if (x0 > x1) {
      int tmp = x1;
      x1 = x0;
      x0 = tmp;
    }
    for (int x = x0; x <= x1; x++) {
      pix[y0 * w + x] = c;
    }
  }
}

static void debug_edge_graph_c(int l, int w, int h, Graph* g, int* c) {
  Color lut[] = {
      WHITE, YELLOW,  BLUE,    PINK,  RED,  PURPLE,
      BLUE,  SKYBLUE, MAGENTA, GREEN, GOLD, VIOLET,
  };
  int num_colors = sizeof(lut) / sizeof(Color);
  int se = g->max_edges;
  int ds = 0;
  int ww = w;
  int hh = h;
  int s = w * h;
  if (l == 1) {
    ds = 2 * h;
    ww = w / 2;
    hh = h / 2;
  }
  Image tmp = GenImageColor(ww, hh, BLACK);
  Color* pix = tmp.data;
  for (int i = 0; i < g->n; i++) {
    int l0, x0, y0, x1, y1, l1;
    int idx0 = g->nodes[i];
    if (idx0 < 0) idx0 = -idx0 - 1;
    l0 = idx0 >= s;
    y0 = (idx0 / ww) - ds;
    x0 = idx0 % ww;
    for (int j = 0; j < g->ecount[i]; j++) {
      int i1 = g->edges[i * se + j].e;
      int idx1 = g->nodes[i1];
      find_idx(s, ww, idx1, &l1, &y1, &x1);
      if (l0 == l1 && l0 == l) {
        assert(c[i1] == c[i]);
        Color cc = lut[c[i1] % num_colors];
        draw_line(ww, x0, y0, x1, y1, pix, cc);
      }
    }
  }
  char fname[100];
  snprintf(fname, sizeof(fname), "c%d.png", l);
  ExportImage(tmp, fname);
  printf("Saved %s.\n", fname);
}

static void draw_line_i(int w, int x0, int y0, int x1, int y1, int* pix,
                        int c) {
  if (x0 == x1) {
    if (y0 > y1) {
      int tmp = y1;
      y1 = y0;
      y0 = tmp;
    }
    for (int y = y0; y <= y1; y++) {
      pix[y * w + x0] = c;
    }
  } else {
    if (x0 > x1) {
      int tmp = x1;
      x1 = x0;
      x0 = tmp;
    }
    for (int x = x0; x <= x1; x++) {
      pix[y0 * w + x] = c;
    }
  }
}

void gen_wire_map(int l, int w, int h, Graph* g, int* comp, int* wmap) {
  int se = g->max_edges;
  int ds = 0;
  int ww = w;
  int hh = h;
  int s = w * h;
  for (int i = 0; i < g->n; i++) {
    int l0, x0, y0, x1, y1, l1;
    int idx0 = g->nodes[i];
    find_idx(s, ww, idx0, &l0, &y0, &x0);
    int ci = comp[i];
    if (l0 == l) wmap[y0 * ww + x0] = ci;
    for (int j = 0; j < g->ecount[i]; j++) {
      int i1 = g->edges[i * se + j].e;
      int idx1 = g->nodes[i1];
      find_idx(s, ww, idx1, &l1, &y1, &x1);
      if (l0 == l1 && l0 == l) {
        draw_line_i(ww, x0, y0, x1, y1, wmap, ci);
      }
    }
  }
}

void wire_graph_init(WireGraph* wg, int nl, int w, int h, PixelGraph* pg,
                     bool debug) {
  profiler_tic_single("wire_graph");
  /* Identify wires/components.  */
  /* Component (wire) for each graph node */
  Graph* g = &pg->g;
  wg->comp = calloc(g->n, sizeof(int));
  wg->nwire = find_connected_components(g, wg->comp);
  if (debug) {
    for (int l = 0; l < nl; l++) {
      debug_edge_graph_c(l, w, h, g, wg->comp);
    }
  }
  /* Generate wire map */
  for (int l = 0; l < nl; l++) {
    int* wm = malloc(w * h * sizeof(int));
    for (int i = 0; i < (w * h); i++) {
      wm[i] = -1;
    }
    gen_wire_map(l, w, h, &pg->g, wg->comp, wm);
    wg->wmap[l] = wm;
  }
  /* Assign drivers and sockets to wires. */
  wg->has_errors = false;
  int ndrv = arrlen(pg->drv);
  int nskt = arrlen(pg->skt);
  int nwire = wg->nwire;
  wg->wire_to_drv = malloc(wg->nwire * sizeof(int));
  wg->wire_to_skt = malloc(nskt * sizeof(SocketDesc));
  wg->wire_to_skt_off = malloc((nwire + 1) * sizeof(int));
  wg->drv_to_wire = malloc(ndrv * sizeof(int));
  wg->skt_to_wire = malloc(nskt * sizeof(int));
  wg->drv_status = malloc(ndrv * sizeof(int));
  wg->skt_status = malloc(nskt * sizeof(int));
  int num_nands = arrlen(pg->nands);
  wg->global_error_flags = 0;
  int* wire_img = wg->wmap[0];
  for (int i = 0; i < ndrv; i++) wg->drv_status[i] = 0;
  for (int i = 0; i < nskt; i++) wg->skt_status[i] = 0;
  for (int i = 0; i < nwire; i++) wg->wire_to_drv[i] = -1;
  int num_nand_drivers = num_nands;
  for (int i = 0; i < ndrv; i++) {
    int idrv = pg->drv[i];
    if (idrv == -1) {
      wg->drv_to_wire[i] = -1;
      continue;
    }
    assert(idrv < w * h);
    int c = wire_img[idrv];
    if (c == -1) {
      /* First error type : Disconnected nand/driver
       * This error is soft on level drivers: If a level driver is unplugged it
       * will still be flagged as error but the error flag won't be set.
       */
      wg->drv_to_wire[i] = -1;
      wg->drv_status[i] = STATUS_DISCONNECTED;
      /* The first #nands drivers are from NANDs, the rest are from the external
       * circuit/level */
      if (i < num_nand_drivers) {
        wg->has_errors = true;
        wg->global_error_flags |= STATUS_DISCONNECTED;
      }
    } else {
      wg->drv_to_wire[i] = c;
      if (wg->wire_to_drv[c] == -1) {
        wg->wire_to_drv[c] = i;
      } else {
        /* Second type of error: 2 drivers pointing to the same wire. */
        wg->drv_status[i] |= STATUS_CONFLICT;
        wg->drv_status[wg->wire_to_drv[c]] |= STATUS_CONFLICT;
        wg->has_errors = true;
        wg->global_error_flags |= STATUS_CONFLICT;
      }
    }
  }
  int num_nand_sockets = 2 * num_nands;
  for (int i = 0; i < nwire + 1; i++) wg->wire_to_skt_off[i] = 0;
  int* cnt = calloc(nwire, sizeof(int));
  for (int i = 0; i < nskt; i++) {
    int idx = pg->skt[i];
    if (idx == -1) {
      wg->skt_to_wire[i] = -1;
      continue;
    }
    int s = wire_img[idx];
    /* Third kind of problem: NAND doesnt have an input. */
    if (s == -1) {
      wg->skt_status[i] |= STATUS_DISCONNECTED;
      wg->global_error_flags |= STATUS_DISCONNECTED;
      /* Again we are more tolerant with level-based drivers */
      if (i < num_nand_sockets) {
        wg->has_errors = true;
      }
    }
    wg->skt_to_wire[i] = s;
    if (s >= 0) {
      assert(s <= nwire);
      wg->wire_to_skt_off[s + 1]++;
    }
  }
  for (int i = 0; i < nwire; i++) {
    wg->wire_to_skt_off[i + 1] += wg->wire_to_skt_off[i];
  }
  for (int i = 0; i < nskt; i++) {
    int idx = pg->skt[i];
    if (idx == -1) {
      continue;
    }
    int s = wire_img[idx];
    if (s >= 0) {
      int off = wg->wire_to_skt_off[s] + cnt[s];
      cnt[s]++;
      wg->wire_to_skt[off] = (SocketDesc){
          .socket = i,
          .dt_ticks = -1,
      };
    }
  }

  profiler_tac_single("wire_graph");
}

void wire_graph_destroy(WireGraph* wg) {
  for (int i = 0; i < MAX_LAYERS; i++) {
    free(wg->wmap[i]);
    wg->wmap[i] = NULL;
  }
  free(wg->wire_to_skt);
  free(wg->wire_to_skt_off);
  free(wg->comp);
  free(wg->wire_to_drv);
  free(wg->drv_to_wire);
  free(wg->skt_to_wire);
  free(wg->drv_status);
  free(wg->skt_status);
}
