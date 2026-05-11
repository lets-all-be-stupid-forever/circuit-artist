#ifndef CA_GRAPH_H
#define CA_GRAPH_H
#include "assert.h"
#include "pq.h"
#include "stb_ds.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
  int e;
  float w;
} GraphEdge;

typedef struct {
  int e;
  int w;
} GraphEdgeInt;

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
                            EdgeGroup* eg, int* layer, float* c_per_w,
                            float* sum_edges);
void djikstra_free(struct Djikstra* dji);

// void djikstra_spanning_tree(Graph* g, int* stack, float* nd, PQ* p, int root,
//                             EdgeGroup* eg);

#if !defined(__cplusplus)
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
#endif

void graph_print_stats(Graph* g);
void graph_print_weights(Graph* g);

typedef struct {
  int src;
  int dst;
  int w;
} GraphBuilderTmp;

typedef struct {
  int nv;
  int* ne_off;
  GraphBuilderTmp* tmp;
} GraphBuilder;

typedef struct {
  int nv;
  int* ne_off;
  GraphEdgeInt* edges;
} GraphL;

void gb_init(GraphBuilder* gb, int nv);
GraphL gb_build_and_dealloc(GraphBuilder* gb);
static inline void gb_add_edge(GraphBuilder* gb, int src, int dst, int d) {
  gb->ne_off[src + 1]++;
  GraphBuilderTmp gt = {
      src,
      dst,
      d,
  };
  arrput(gb->tmp, gt);
}

void debug_graphl(GraphL* g);
GraphL graphl_invert(GraphL* g);
void graphl_free(GraphL* g);

bool longest_distance(GraphL* gfwd, GraphL* gbwd, int* AT, int* RAT,
                      int* prv_AT);

/* Strongly Connected Components algorithm.
 *
 * Path-based, implementation from:
 * https://en.wikipedia.org/wiki/Path-based_strong_component_algorithm
 * Uses an iterative DFS algorithm.
 *
 * Args:
 * g: (Input)
 *   Directed graph. (N vertices)
 * comp: (Output)
 *    Array of size N. (alloced by the caller)
 *    The assigned component number of each vertex.
 * compsz: (Output)
 *    Array of size N . (alloced by the caller)
 *    The size of each strongly connected component. Helps on finding the nodes
 *    that don't belong to a cycle: Those with size=1 are nodes outside a
 *    cycle.
 *
 * Return:
 *   The number of connected components found (nc).
 *
 * */
int strongly_connected_components(GraphL* g, int* comp, int* compsz);

/* Connected Components algorithm.
 *
 * Assumes undirected graph.
 *
 * Args:
 *   g: (Input)
 *      Input Graph. (N nodes)
 *   c:  (Output)
 *      Array of size N. (allocated by the caller)
 *      Stores the component number of each node.
 *
 * Returns the number of connected components found.
 *
 * */
int find_connected_components(Graph* g, int* c);

#if defined(__cplusplus)
}
#endif

#endif
