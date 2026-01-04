#include "graph.h"

static inline void edge_group_add_direct_edge(EdgeGroup* eg, int s, int t,
                                              float w) {
  eg->ne++;
  int e = eg->ecount[s]++;
  int off = s * eg->max_edges;
  eg->edges[off + e] = (GraphEdge){
      .e = t,
      .w = w,
  };
}

static inline void edge_group_add_edge(EdgeGroup* eg, int s, int t, float w) {
  edge_group_add_direct_edge(eg, s, t, w);
  edge_group_add_direct_edge(eg, t, s, w);
}

EdgeGroup* edge_group_create(int max_edges) {
  EdgeGroup* eg = calloc(1, sizeof(EdgeGroup));
  eg->cap = 100;
  eg->n = 0;
  eg->ne = 0;
  eg->max_edges = max_edges;
  eg->edges = calloc(eg->max_edges * eg->cap, sizeof(GraphEdge));
  eg->ecount = calloc(eg->cap, sizeof(int));
  return eg;
}

void edge_group_alloc(EdgeGroup* eg, int n) {
  eg->ne = 0;
  eg->n = n;
  while (eg->n > eg->cap - 10) {
    eg->cap *= 2;
    eg->edges = realloc(eg->edges, eg->max_edges * eg->cap * sizeof(GraphEdge));
    eg->ecount = realloc(eg->ecount, eg->cap * sizeof(int));
  }
  for (int i = 0; i < n; i++) {
    eg->ecount[i] = 0;
  }
}

void edge_group_free(EdgeGroup* eg) {
  if (eg) {
    free(eg->edges);
    free(eg->ecount);
    free(eg);
  }
}

struct Djikstra {
  PQ pq;
  bool* done;
  float* dist;
};

struct Djikstra* djikstra_create() {
  struct Djikstra* dji = calloc(1, sizeof(struct Djikstra));
  pq_init(&dji->pq);
  return dji;
}

void djikstra_spanning_tree(struct Djikstra* dj, Graph* g, int root,
                            EdgeGroup* eg, int* layer, float* c_per_w,
                            float* sum_edges) {
  assert(root >= 0);
  edge_group_alloc(eg, g->n);
  arrsetlen(dj->dist, 0);
  arrsetlen(dj->done, 0);
  int n = g->n;
  for (int i = 0; i < n; i++) {
    arrput(dj->dist, -1.f);
    arrput(dj->done, false);
  }
  dj->done[root] = 0;
  PQ* p = &dj->pq;
  float sum = 0;
  pq_push(p, -1, root, 0);
  while (p->size > 0) {
    int u = pq_top(p).dst;
    int prev = pq_top(p).src;
    pq_pop(p);
    if (dj->done[u]) continue;
    dj->done[u] = true;
    int ne = g->ecount[u];
    int off = g->max_edges * u;
    float cw = c_per_w[layer[u]];
    for (int ie = 0; ie < ne; ie++) {
      int v = g->edges[off + ie].e;
      float w = g->edges[off + ie].w;
      float ew = cw * w;
      sum += ew;
      if (v == prev) {
        edge_group_add_edge(eg, prev, u, w);
      } else {
        float alt = dj->dist[u] + ew;
        if ((alt < dj->dist[v]) || (dj->dist[v] < 0)) {
          dj->dist[v] = alt;
          pq_push(p, u, v, -alt);
        }
      }
    }
  }
  /* Each edge is counted twice */
  *sum_edges = sum / 2.f;
}

void djikstra_free(struct Djikstra* dji) {
  if (!dji) return;
  arrfree(dji->done);
  arrfree(dji->dist);
  free(dji);
}

void graph_print_weights(Graph* g) {
  int se = 0;
  for (int i = 0; i < g->n; i++) {
    printf("Node %d: \n", i);
    int ne = g->ecount[i];
    int me = g->max_edges;
    for (int e = 0; e < ne; e++) {
      int j = g->edges[me * i + e].e;
      float w = g->edges[me * i + e].w;
      printf("%d->%d : %f\n", i, j, w);
    }
  }
  printf("num_nodes=%d num_edges=%d\n", g->n, se);
}

void graph_print_stats(Graph* g) {
  int se = 0;
  for (int i = 0; i < g->n; i++) {
    se += g->ecount[i];
  }
  printf("num_nodes=%d num_edges=%d\n", g->n, se);
}

