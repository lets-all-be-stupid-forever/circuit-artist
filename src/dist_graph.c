#include "dist_graph.h"

#include "elmore.h"
#include "img.h"
#include "math.h"
#include "pq.h"
#include "profiler.h"

static inline int mini(int a, int b) { return a < b ? a : b; }
static inline int maxi(int a, int b) { return a > b ? a : b; }
static inline int maxf(float a, float b) { return a > b ? a : b; }

static inline void iswap(int* a, int* b) {
  int t = *a;
  *a = *b;
  *b = t;
}

static inline void fswap(float* a, float* b) {
  float t = *a;
  *a = *b;
  *b = t;
}

/*
 * Approximation of Elmore delay, transforming the simple pixel distance to a
 * L^2-based distance for delay purposes.
 * It's basically a constant times the distance (in segments) square. The
 * parameter is then calibrated "empirically" so it stays fun to play/optimize.
 * The key idea is to have a L^2-based delay instead of L-based so the user
 * will consider buffers, smaller/compact wires (layers), and use upper layers
 * for faster speed.
 */
static inline int dist2delay(float alpha, int d) {
  float d2 = ((float)d);
  d2 = alpha * d2 * d2;
  return d2 + 1;
}

/*
 * Extracts a subgraph from a bigger graph using specific nodes.
 */
static void dist_build_subgraph(Graph* g,    /* Original graph (input) */
                                Graph* subg, /* new subgraph (output)*/
                                int nk,      /* number of nodes to extract */
                                int* nodes   /* nodes INDEXES to extract */
) {
  graph_reset(subg);
  if (nk == 0) {
    return;
  }
  for (int k = 0; k < nk; k++) {
    graph_add_node(subg, nodes[k]);
  }
  for (int k = 0; k < nk; k++) {
    int i0 = nodes[k];
    int ne = g->ecount[i0];
    GraphEdge* p_edges = g->edges + i0 * g->max_edges;
    for (int ek = 0; ek < ne; ek++) {
      int k1;
      int i2_e = p_edges[ek].e;
      float i2_w = p_edges[ek].w;
      graph_node_inv(subg, i2_e, &k1);
      // if (k1 < k) { /* Avoids adding directed edges twice*/
      graph_add_direct_edge(subg, k, k1, i2_w);
      //}
    }
  }
}

/*
 * Extracts the nodes from each connected component in an ordered list.
 * Attention: it sorts using node index, and not node value!
 */
static void group_nodes_by_component(
    Graph* g,  /* Graph to extract */
    int nc,    /* Num of components */
    int* comp, /* Component of each node (size N)*/
    int* off,  /* (output) offset of each component (size NC+1)*/
    int* nodes /* (output) INDEX of each original node */
) {
  int* nu = calloc(nc, sizeof(int));
  for (int i = 0; i < g->n; i++) {
    off[comp[i] + 1]++;
  }
  for (int i = 0; i < nc; i++) {
    off[i + 1] += off[i];
  }
  for (int i = 0; i < g->n; i++) {
    int c = comp[i];
    nodes[off[c] + nu[c]++] = i;
  }
  free(nu);
}

static inline float interp(int p0, int p1, int p, float rc, float d0,
                           float d1) {
  float a, t0, t1;
  if (p0 == p) return d0;
  if (p1 == p) return d1;
  float fp0 = p0;
  float fp1 = p1;
  float fp = p;
  if (d0 < d1) {
    a = (fp - fp0) / (fp1 - fp0);
    t0 = d0;
    t1 = d1;
  } else {
    a = (fp1 - fp) / (fp1 - fp0);
    t0 = d1;
    t1 = d0;
  }
  return t0 + (t1 - t0) * a + rc * a * (1.f - a);
}
/*
 * Creates a distance map from the distance of each node in a graph.
 */
static void setup_dist_map(
    DistSpec spec, bool lone, int w0, /* Width of base image */
    int h0,                           /* Height of base image */
    Graph* g,                         /* Graph with distance nodes */
    float* node_dist,                 /* Distance of each node */
    float** distmap,                  /* Output distance map for each layer */
    int ic,                           /* Component being treated */
    WireSegment** seglist, /* List of wire segments used in visualization*/
    u8** ori,              /* orientation of each pixel */
    float* out_maxdist     /* Maximum distance */
) {
  int l0, xx0, yy0; /* Layer, x and y index of the node */
  int l1, x1, y1;   /* Layer, x and y index of the node */
  int s = w0 * h0;
  // int w1 = w0;  // / 2;
  // int h1 = h0;  // / 2;
  float max_dist = 0;
  // TODO: Fix it
  for (int i = 0; i < g->n; i++) {
    int ne = g->ecount[i];
    int off = g->max_edges * i;
    float dd0 = node_dist[i];
    int idx0 = g->nodes[i];
    find_idx(s, w0, idx0, &l0, &yy0, &xx0);
    // int w = l0 == 0 ? w0 : w1;
    int w = w0;
    float* map = distmap[l0];
    u8* l_ori = ori[l0];
    // Case (i): single pixel with no edges
    if (ne == 0) {
      WireSegment ws = (WireSegment){ic, yy0 * w + xx0, yy0 * w + xx0};
      arrput((seglist[l0]), ws);
    }
    float rc = spec.r_per_w[l0] * spec.c_per_w[l0];
    for (int ie = 0; ie < ne; ie++) {
      int e = g->edges[off + ie].e;
      float ew = g->edges[off + ie].w;
      float rc_seg = 0.5 * ew * ew * rc;
      int idx1 = g->nodes[e];
      float dd1 = node_dist[e];
      float d0 = dd0;
      float d1 = dd1;
      int x0 = xx0;
      int y0 = yy0;
      find_idx(s, w0, idx1, &l1, &y1, &x1);
      if (l0 != l1) {
        // Case (ii): edge connects different layers - add 1-pixel seg at each
        // layer
        WireSegment ws0 = (WireSegment){ic, y0 * w + x0, y0 * w + x0};
        arrput((seglist[l0]), ws0);
        int w1 = w0;  // assuming same width for all layers
        WireSegment ws1 = (WireSegment){ic, y1 * w1 + x1, y1 * w1 + x1};
        arrput((seglist[l1]), ws1);
        continue;
      }
      /* positive indexes are from hseg
       * negative indexes are from vseg
       * So, I don't want to add a seg if it's vias
       */
      if ((idx0 >= 0 && idx1 >= 0) || (idx0 < 0 && idx1 < 0)) {
        WireSegment ws = (WireSegment){ic, y0 * w + x0, y1 * w + x1};
        assert((y0 == y1) || (x0 == x1));
        arrput((seglist[l0]), ws);
      }

      max_dist = maxf(max_dist, d0);
      max_dist = maxf(max_dist, d1);
      if (y1 < y0) {
        iswap(&y0, &y1);
        fswap(&d0, &d1);
      }
      if (x1 < x0) {
        iswap(&x0, &x1);
        fswap(&d0, &d1);
      }
      float t, t0, t1, a;
      if (x1 == x0) {
        for (int y = y0; y <= y1; y++) {
          int idx = y * w + x0;
          u8 v = l_ori[idx];
          if (v == 1) {
            /* Vertical distances are negative */
            map[idx] = lone ? 0 : -interp(y0, y1, y, rc_seg, d0, d1);
          }
        }
      }
      if (y1 == y0) {
        for (int x = x0; x <= x1; x++) {
          int idx = y0 * w + x;
          u8 v = l_ori[idx];
          float d = interp(x0, x1, x, rc_seg, d0, d1);
          // if (map[idx] == 0) {
          if (v == 0) {
            map[idx] = lone ? 0 : d;
          }
        }
      }
    }
  }
  int max_delay = ceilf(max_dist + 1);
  *out_maxdist = max_delay;
}

static Image int2img(int w, int h, int* p) {
  Image out = GenImageColor(w, h, BLACK);
  Color* pix = out.data;
  for (int i = 0; i < (w * h); i++) {
    int v = p[i];
    pix[i] = (Color){v, v, v, 255};
  }
  return out;
}

static Image float2img(int w, int h, float f, float* p) {
  Image out = GenImageColor(w, h, BLACK);
  Color* pix = out.data;
  for (int i = 0; i < (w * h); i++) {
    float v = p[i];
    pix[i] = (Color){f * v, f * v, f * v, 255};
  }
  return out;
}

static void dist_graph_debug(DistGraph* dg, int nl, int w, int h) {
  for (int l = 0; l < nl; l++) {
    char fname[50];
    snprintf(fname, sizeof(fname), "d%d.png", l);
    ExportImage(float2img(w, h, 100, dg->distmap[l]), fname);
    printf("Saved %s\n", fname);
  }
}

static inline int find_node_from_idx(Graph* g, Graph* gg, int idx) {
  int hn = -1;
  int vn = -1;
  graph_node_inv(g, idx, &hn);
  graph_node_inv(g, -idx - 1, &vn);
  if (hn != -1) {
    graph_node_inv(gg, hn, &hn);
  }
  if (vn != -1) {
    graph_node_inv(gg, vn, &vn);
  }
  int ret = hn;
  if (ret == -1) ret = vn;
  return ret;
}

static inline int find_layer(int s, int idx) {
  if (idx < 0) idx = -idx - 1;
  if (idx < 1 * s) return 0;
  if (idx < 2 * s) return 1;
  if (idx < 3 * s) return 2;
  return -1;
}

static inline int find_gate_delay(DistSpec* spec, float ctotal) {
  float fixed_gate_delay = spec->fixed_gate_delay;
  float R_gate = spec->r_gate;
  double c_gate = spec->c_gate;
  return (int)ceilf(R_gate * (ctotal + 0.5 * c_gate) + fixed_gate_delay);
}

static inline float find_pulse_energy(DistSpec* spec, float ctotal) {
  return 0.5 * ctotal * spec->vdd * spec->vdd;
}

/*
 * Computes the distance-to-driver for each wire.
 * This distance is then used to compute wire delay and for wire propagation
 * animation.
 *
 * The output will be a distance map image for each layer and a list of
 * segments that can be used in visualization update.
 */
void dist_graph_init(DistGraph* dg, DistSpec spec, int w, int h, int nl,
                     Graph* g, /* Pixel graph */
                     int* wire_to_drv, int* p_drv, SocketDesc* wire_to_skt,
                     int* wire_to_skt_off, int* p_skt, int n_skt, int nc,
                     int* comp, u8** ori, bool debug) {
  profiler_tic_single("dist_graph");
  /* I need to be able to identify all nodes for each component, then I re-build
   * the graph.*/
  PQ pq = {0};
  for (int l = 0; l < nl; l++) {
    dg->distmap[l] = calloc(w * h, sizeof(float));
  }
  dg->wprop = calloc(nc, sizeof(WireProps));
  dg->gate_delay = calloc(nc, sizeof(int));

  /* Offset node per wire */
  int* noff = calloc((nc + 1), sizeof(int));
  /* sorted nodes */
  int* n2 = calloc(g->n, sizeof(int));
  /* temporary buffer used in dist calculation */
  float* node_distance = calloc(g->n + n_skt, sizeof(float));
  /* temporary buffer used in dist calculation */
  int* stack = calloc(g->n, sizeof(int));
  Graph gg = {0};
  graph_init(&gg, 20);
  pq_init(&pq);
  group_nodes_by_component(g, nc, comp, noff, n2);
  int w0 = w;
  int h0 = h;
  EdgeGroup* eg = edge_group_create(g->max_edges);
  ElmoreCalculator* ec = elmore_calculator_create();
  ec->phys = spec;
  struct Djikstra* dj = djikstra_create();

  dg->t_setup = 0;
  dg->t_build = 0;
  dg->t_elmore = 0;

  double vdd = spec.vdd;
  //   double c_w = spec.c_per_w;
  int* layer = calloc(g->n, sizeof(int));

  /* Resistance of the gate, used for calculation of gate activation delay */
  double t0; /* used for profiling */
  int img_size = w * h;
  for (int c = 0; c < nc; c++) {
    t0 = GetTime();
    /* Creates a subgraph so we have better cache performance. */
    int nk = noff[c + 1] - noff[c];
    int* subnodes = &n2[noff[c]];
    dist_build_subgraph(g, &gg, nk, subnodes);
    for (int ik = 0; ik < nk; ik++) {
      layer[ik] = find_layer(img_size, g->nodes[subnodes[ik]]);
    }

    /*printf("subg:\n ");*/
    /*print_graph(&gg);*/
    /* Extracts index of root of the wire (vertical and horizontal) */
    // int hdrv = -1;
    //    int vdrv = -1;
    int root = -1;
    int drv = wire_to_drv[c];
    if (drv != -1) {
      int idx = p_drv[drv];
      root = find_node_from_idx(g, &gg, idx);
      assert(root >= 0);
    }

    /* Adds virtual nodes for the sockets associated with this wire */
    int skt_off = wire_to_skt_off[c];
    int nskt = wire_to_skt_off[c + 1] - skt_off;
    int s_off = 1 << 30;
    for (int iskt = 0; iskt < nskt; iskt++) {
      int skt = wire_to_skt[skt_off + iskt].socket;
      int skt_idx = p_skt[skt];
      int node = find_node_from_idx(g, &gg, skt_idx);
      assert(node >= 0);
      int r = graph_add_node(&gg, s_off + skt);
      layer[r] = 0;            /* Nands are always at the layer 0 */
      float w = spec.l_socket; /* Wire length associated to a nand input */
      graph_add_edge(&gg, node, r, w);
    }

    dg->t_build += GetTime() - t0;
    t0 = GetTime();

    /* Distance calculation in graph */
    bool lone = true;
    if (root >= 0) {
      lone = false;
      int* _ecount = gg.ecount;
      GraphEdge* _edges = gg.edges;
      float c_cyclic_total = -1;
      if (!graph_is_tree(&gg)) {
        float sum_edges = 0;
        djikstra_spanning_tree(dj, &gg, root, eg, layer, spec.c_per_w,
                               &sum_edges);
        c_cyclic_total = sum_edges;
        assert(edge_group_is_tree(eg));
        _ecount = eg->ecount;
        _edges = eg->edges;
#if 0
        printf("after prune:\n");
        {
          int se = 0;
          int me = eg->max_edges;
          for (int i = 0; i < eg->n; i++) {
            printf("Node %d: \n", i);
            int ne = eg->ecount[i];
            for (int e = 0; e < ne; e++) {
              int j = eg->edges[me * i + e].e;
              float w = g->edges[me * i + e].w;
              printf("%d->%d : %f\n", i, j, w);
            }
          }
          printf("num_nodes=%d num_edges=%d\n", eg->n, eg->ne);
        }
#endif
      }
      elmore_calculator_run(ec, gg.n, gg.max_edges, _ecount, layer, _edges,
                            root, node_distance);
      /* The ctotal is used for the NAND activation time, ie, before the change
       * actual starts. */
      float ctotal = ec->ctotal;
      float f = 1.f;
      if (c_cyclic_total > 0) {
        f = c_cyclic_total / ctotal;
      }
      ctotal = ctotal * f;
      /*
       * A      B
       * |------|----- - - -
       *   Gate    Wire
       *
       * c_down(B) = cGRAPH_down(B) + 0.5 * c_gate.
       * T_D(B) = r_gate * c_down(B)
       */
      /* Gate delay depends only on total capacitance.
       * Gate delay is the time for the NAND gate to activate (before
       * propagation)
       * */
      dg->gate_delay[c] = find_gate_delay(&spec, ctotal);

      assert(ctotal >= 0);
      /* Pulse energy is separated from gate/driver energy */
      // printf("nk %d ctotal %f\n", nk, ctotal);
      dg->wprop[c].pulse_energy = find_pulse_energy(&spec, ctotal);

      if (f > 1.f) {
        for (int k = 0; k < gg.n; k++) {
          node_distance[k] *= f;
        }
      }
      for (int k = 0; k < gg.n; k++) {
        assert(node_distance[k] >= 0);
      }
    } else {
      /* No driver = gate delay of 0 */
      dg->wprop[c].pulse_energy = spec.lone_pulse_energy;
      dg->gate_delay[c] = 2;
      for (int k = 0; k < gg.n; k++) {
        node_distance[k] = 0;
      }
    }

    /* removes added sockets and updates socket distance */
    for (int iskt = nskt - 1; iskt >= 0; iskt--) {
      int s = graph_pop_node(&gg);
      float dist_f = node_distance[s];
      if (dist_f < 0) dist_f = -dist_f;
      int dist_i = (int)(floorf(dist_f));
      wire_to_skt[skt_off + iskt].dt_ticks = dist_i;
    }

    /* Transforms subg node from node-index to image-index */
    for (int k = 0; k < nk; k++) {
      // assert(node_distance[k] >= 0);
      // printf("d[%d]=%f\n", k, node_distance[k]);
      gg.nodes[k] = g->nodes[subnodes[k]];
    }
    dg->t_elmore += GetTime() - t0;
    t0 = GetTime();
#if 0
    if (gg.n > 1) {
      for (int i = 0; i < gg.n; i++) {
        printf("dist[%d]=%f\n", i, node_distance[i]);
      }
    }
#endif
    /* From graph to 2D distance map And wire segments */
    setup_dist_map(spec, lone, w0, h0, &gg, node_distance, dg->distmap, c,
                   dg->seglist, ori, &dg->wprop[c].max_delay);
    dg->t_setup += GetTime() - t0;
    t0 = GetTime();
  }
  printf("time_build  = %.1f ms\n", (dg->t_build * 1000));
  printf("time_elmore = %.1f ms\n", (dg->t_elmore * 1000));
  printf("time_setup  = %.1f ms\n", (dg->t_setup * 1000));
  /*
   * Big img debug mode (no layer) :
  t_build  = 367.7 ms
  t_elmore = 27.0 ms
  t_setup  = 152.6 ms

   * Big img release mode (no layer):
   * Maybe it's the find_idx that is very slow?
   t_build  = 169.2 ms
   t_elmore = 11.9 ms
   t_setup  = 83.9 ms
   [pixel_graph]   81.3ms    0.5ms   78.2ms  217.8ms    1.2ms (0 layer)
   [pixel_graph]   84.1ms    0.4ms  159.3ms  218.6ms    2.3ms (+1 layer)

   Hanoi release:
   t_build  = 6.2 ms
   t_elmore = 0.5 ms
   t_setup  = 3.5 ms
  */

  if (debug) {
    dist_graph_debug(dg, nl, w, h);
  }

  djikstra_free(dj);
  elmore_calculator_free(ec);
  edge_group_free(eg);
  pq_destroy(&pq);
  free(node_distance);
  free(stack);
  free(n2);
  free(noff);
  free(layer);
  profiler_tac_single("dist_graph");
  // save_img_f32(w, h, dg->distmap[0], -40, 40, "../dmap.png");
}

void dist_graph_destroy(DistGraph* dg) {
  free(dg->wprop);
  free(dg->gate_delay);
  for (int i = 0; i < MAX_LAYERS; i++) {
    free(dg->distmap[i]);
    arrfree(dg->seglist[i]);
  }
}

