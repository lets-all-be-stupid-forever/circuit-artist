#ifndef CA_GRAPH_H
#define CA_GRAPH_H
#include "assert.h"
#include "pq.h"
#include "stb_ds.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"

typedef struct {
  int e;
  float w;
} GraphEdge;

typedef struct {
  int n;
  int ne;
  int max_edges;  // max edges
  GraphEdge* edges;
  int* ecount;
  int cap;
} EdgeGroup;

EdgeGroup* edge_group_create(int max_edges);
void edge_group_alloc(EdgeGroup* eg, int n);
void edge_group_free(EdgeGroup* eg);

typedef struct {
  struct {
    int key;
    int value;
  }* inv;
  int* nodes;
  GraphEdge* edges;
  int* ecount;
  int ne;
  int n;
  int max_edges;  // max edges
  int cap;
} Graph;

struct Djikstra;

struct Djikstra* djikstra_create();
void djikstra_spanning_tree(struct Djikstra* dji, Graph* g, int root,
                            EdgeGroup* eg, float* sum_edges);
void djikstra_free(struct Djikstra* dji);

// void djikstra_spanning_tree(Graph* g, int* stack, float* nd, PQ* p, int root,
//                             EdgeGroup* eg);

static inline bool graph_is_tree(Graph* g) { return g->ne == 2 * (g->n - 1); }
static inline bool edge_group_is_tree(EdgeGroup* eg) {
  return eg->ne == 2 * (eg->n - 1);
}

static inline int graph_pop_node(Graph* g) {
  int s = g->n - 1;
  int ne = g->ecount[s];
  int me = g->max_edges;
  for (int i = 0; i < ne; i++) {
    int v = g->edges[s * me + i].e;
    g->ecount[v]--;
  }
  g->n--;
  return s;
}

static inline void graph_add_direct_edge(Graph* g, int s, int t, float w) {
  g->ne++;
  int e = g->ecount[s]++;
  int off = s * g->max_edges;
  g->edges[off + e] = (GraphEdge){
      .e = t,
      .w = w,
  };
}

static inline void graph_add_edge(Graph* g, int s, int t, float w) {
  assert(s != t);
  graph_add_direct_edge(g, s, t, w);
  graph_add_direct_edge(g, t, s, w);
}

static inline GraphEdge graph_get_neighboor(Graph* g, int s, int i) {
  assert(g->ecount[s] > i);
  return g->edges[g->max_edges * s + i];
}

static inline bool graph_node_inv(Graph* g, int nv, int* v) {
  int i = hmgeti(g->inv, nv);
  if (i < 0) return false;
  *v = g->inv[i].value;
  return true;
}

static inline int graph_add_node(Graph* g, int nv) {
  int r = g->n++;
  g->nodes[r] = nv;
  g->ecount[r] = 0;
  hmput(g->inv, nv, r);
  while (g->n > g->cap - 10) {
    g->cap *= 2;
    g->edges = realloc(g->edges, g->max_edges * g->cap * sizeof(GraphEdge));
    g->nodes = realloc(g->nodes, g->cap * sizeof(int));
    g->ecount = realloc(g->ecount, g->cap * sizeof(int));
  }
  return r;
}

static void graph_init(Graph* g, int max_edges) {
  *g = (Graph){0};
  g->max_edges = max_edges;
  g->n = 0;
  g->ne = 0;
  g->cap = 1000;
  g->edges = calloc(max_edges * g->cap, sizeof(GraphEdge));
  g->ecount = calloc(g->cap, sizeof(int));
  g->nodes = calloc(g->cap, sizeof(int));
}

static void graph_reset(Graph* g) {
  g->ne = 0;
  g->n = 0;
  // TODO remove this hack!!
  while (hmlen(g->inv) > 0) {
    hmdel(g->inv, g->inv[0].key);
  }
}

static void graph_destroy(Graph* g) {
  free(g->edges);
  free(g->nodes);
  free(g->ecount);
  hmfree(g->inv);
}

void graph_print_stats(Graph* g);
void graph_print_weights(Graph* g);

#endif
