#include "connected_components.h"

// Union-find Find
static inline int uf_find(int* c, int a) {
  int root = a;
  while (c[root] != root) {
    root = c[root];
  }
  while (c[a] != root) {
    int par = c[a];
    c[a] = root;
    a = par;
  }
  return root;
}

// Union-find Union
static inline void uf_union(int* c, int* r, int a, int b) {
  int ca = uf_find(c, a);
  int cb = uf_find(c, b);
  if (ca == cb) {
    return;
  }
  if (r[ca] < r[cb]) {
    c[ca] = cb;
  } else {
    c[cb] = ca;
    if (r[ca] == r[cb]) {
      r[ca] += 1;
    }
  }
}

int find_connected_components(Graph* g, int* c) {
  /* uf rank */
  int n = g->n;
  int* r = calloc(n, sizeof(int));
  int* cc = calloc(n, sizeof(int));
  for (int i = 0; i < n; i++) {
    cc[i] = i;
  }
  int me = g->max_edges;
  for (int i = 0; i < n; i++) {
    GraphEdge* ee = &g->edges[i * me];
    int ne = g->ecount[i];
    for (int e = 0; e < ne; e++) {
      uf_union(cc, r, i, ee[e].e);
    }
  }
  int k = 0;
  for (int i = 0; i < n; i++) {
    r[i] = -1;
  }
  // Flat-up the components
  for (int i = 0; i < n; i++) {
    cc[i] = uf_find(cc, i);
    int ri = r[cc[i]];
    if (ri == -1) {  // new component
      r[cc[i]] = k;
      c[i] = k;
      k++;
    } else {
      c[i] = ri;
    }
  }
  free(cc);
  free(r);
  return k;
}
