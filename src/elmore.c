#include "elmore.h"

#include "assert.h"
#include "graph.h"
#include "stdlib.h"

ElmoreCalculator* elmore_calculator_create() {
  ElmoreCalculator* e = calloc(1, sizeof(ElmoreCalculator));
  e->cap = 100;
  e->cdown = malloc(e->cap * sizeof(float));
  e->sorted = malloc(e->cap * sizeof(int));
  e->parent = malloc(e->cap * sizeof(ParentEdge));
  return e;
}

void elmore_calculator_free(ElmoreCalculator* e) {
  if (!e) return;
  free(e->cdown);
  free(e->sorted);
  free(e->parent);
  free(e);
}

static void elmore_alloc_buffers(ElmoreCalculator* ec, int n) {
  while (n > ec->cap - 5) {
    ec->cap *= 2;
    ec->cdown = realloc(ec->cdown, ec->cap * sizeof(float));
    ec->sorted = realloc(ec->sorted, ec->cap * sizeof(int));
    ec->parent = realloc(ec->parent, ec->cap * sizeof(ParentEdge));
  }
}

void elmore_calculator_run(ElmoreCalculator* e, int n, int me, int* ecount,
                           GraphEdge* edges, int root, float* node_distance) {
  elmore_alloc_buffers(e, n);

  /* First DFS to estimate downstream capacitance */
  int top = -1;
  e->sorted[++top] = root;
  e->parent[root] = (ParentEdge){-1, -1};
  for (int i = 0; i < n; i++) {
    int u = e->sorted[i];
    int ne = ecount[u];
    int off = me * u;
    float myc = 0;
    int par = e->parent[u].v;
    for (int ie = 0; ie < ne; ie++) {
      int v = edges[off + ie].e;
      float w = edges[off + ie].w;
      assert(w >= 0);
      myc += w * e->c_per_w;
      if (v != par) {
        e->parent[v] = (ParentEdge){u, w};
        e->sorted[++top] = v;
      }
    }
    e->cdown[u] = myc / 2.f;
    assert(e->cdown[u] >= 0);
  }

  /* Downstream C calculation */
  for (int i = n - 1; i > -1; i--) {
    int u = e->sorted[i];
    int p = e->parent[u].v;
    if (p >= 0) {
      e->cdown[p] += e->cdown[u];
    }
  }

  /* Elmore Delay calculation */
  node_distance[root] = 0;
  for (int i = 0; i < n; i++) {
    int u = e->sorted[i];
    int p = e->parent[u].v;
    float w = e->parent[u].w;
    if (p >= 0) {
      assert(w >= 0);
      node_distance[u] = node_distance[p] + e->r_per_w * w * e->cdown[u];
    }
  }

  /* ctotal is used to compute the nand activation delay */
  e->ctotal = e->cdown[root];
}
