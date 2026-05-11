#include "graph.h"

#include "stdio.h"
#include "union_find.h"

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
void gb_init(GraphBuilder* gb, int nv) {
  gb->ne_off = calloc(nv + 1, sizeof(int));
  gb->nv = nv;
  assert(gb->tmp == NULL);
}

GraphL gb_build_and_dealloc(GraphBuilder* gb) {
  /* Transforming count in offset, and initializing index
   * counter */
  int* ne_off = gb->ne_off;
  int nv = gb->nv;
  int* ne = calloc(nv, sizeof(int)); /*edges LEAVING node*/
  for (int i = 0; i < nv; i++) {
    ne[i] = 0;
    ne_off[i + 1] += ne_off[i];
  }

  /* Second pass: mounts the actual graph */
  int nt = arrlen(gb->tmp);
  assert(gb->ne_off[nv] == nt);
  GraphEdgeInt* edges = calloc(nt, sizeof(GraphEdge));
  for (int i = 0; i < nt; i++) {
    int v = gb->tmp[i].src;
    int off = ne_off[v] + ne[v]++;
    edges[off] = (GraphEdgeInt){
        .e = gb->tmp[i].dst,
        .w = gb->tmp[i].w,
    };
  }
  free(ne);
  arrfree(gb->tmp);
  gb->tmp = NULL;
  gb->ne_off = NULL;
  return (GraphL){.nv = nv, .ne_off = ne_off, .edges = edges};
}

void debug_graphl(GraphL* g) {
  for (int i = 0; i < g->nv; i++) {
    int off = g->ne_off[i];
    int ne = g->ne_off[i + 1] - off;
    for (int j = 0; j < ne; j++) {
      GraphEdgeInt ge = g->edges[off + j];
      printf("%4d -->> %-4d (%d)\n", i, ge.e, ge.w);
    }
  }
}

GraphL graphl_invert(GraphL* g) {
  GraphBuilder gb = {0};
  gb_init(&gb, g->nv);
  for (int i = 0; i < g->nv; i++) {
    int off = g->ne_off[i];
    int ne = g->ne_off[i + 1] - off;
    for (int j = 0; j < ne; j++) {
      GraphEdgeInt ge = g->edges[off + j];
      gb_add_edge(&gb, ge.e, i, ge.w);
    }
  }
  return gb_build_and_dealloc(&gb);
}

void graphl_free(GraphL* g) {
  free(g->edges);
  free(g->ne_off);
  g->edges = NULL;
  g->ne_off = NULL;
}

// Computes AT and RAT for a directed acyclic graph (DAG)
// Returns false if graph is not acyclic
bool longest_distance(GraphL* gfwd, GraphL* gbwd, int* AT, int* RAT,
                      int* prv_AT) {
  int nv = gfwd->nv;
  int* q = calloc(nv, sizeof(int));
  int nq = 0;
  int qi = 0;
  int* indeg = calloc(nv, sizeof(int));

  /* Initialize AT with 0 */
  for (int i = 0; i < nv; i++) {
    AT[i] = 0;
    prv_AT[i] = i;
  }

  // I need to find components...
  // int* CP = calloc(nv, sizeof(int));

  /* First I compute indegree of each node (indeg).
   * Then I queue those with indeg=0 */
  for (int i = 0; i < nv; i++) {
    indeg[i] = gbwd->ne_off[i + 1] - gbwd->ne_off[i];
    if (indeg[i] == 0) q[nq++] = i;
  }

  /* Now I iterate over those with indeg=0 */
  int max_AT = 0;
  while (qi != nq) {
    int v = q[qi++];
    int c = AT[v];
    AT[v] = -AT[v] - 1; /* Means it has been visited */
    int off = gfwd->ne_off[v];
    int ne = gfwd->ne_off[v + 1] - gfwd->ne_off[v];
    for (int i = 0; i < ne; i++) {
      int e = gfwd->edges[off + i].e;
      /* note: We can't visit the same node twice here because every visited
       * node has indeg=0, meaning it was already visited by all its neighbors
       * Cycles manifest by nodes not being visited (with indeg >0).
       * */
      int w = gfwd->edges[off + i].w;
      int cc = c + w;
      int ce = AT[e];
      if (ce < cc) {
        /* Critical path */
        max_AT = max_AT > cc ? max_AT : cc;
        AT[e] = cc;
        prv_AT[e] = v;
      }
      indeg[e]--;
      if (indeg[e] == 0) {
        q[nq++] = e;
      }
    }
  }
  /* The rest of nodes that weren't processed won't enter in the calculations */

  /* Fixes signal*/
  for (int i = 0; i < nv; i++) {
    if (AT[i] < 0) {
      AT[i] = -AT[i] - 1;
      RAT[i] = max_AT;
    } else {
      AT[i] = -1;
      RAT[i] = 1000000;
    }
  }

  /* Now computing the RAT using the inversed graph */
  nq = 0;
  for (int i = 0; i < nv; i++) {
    indeg[i] = gfwd->ne_off[i + 1] - gfwd->ne_off[i];
    if (indeg[i] == 0) q[nq++] = i;
  }

  qi = 0;
  while (qi != nq) {
    int v = q[qi++];
    int c = RAT[v];
    int off = gbwd->ne_off[v];
    int ne = gbwd->ne_off[v + 1] - gbwd->ne_off[v];
    for (int i = 0; i < ne; i++) {
      int e = gbwd->edges[off + i].e;
      int w = gbwd->edges[off + i].w;
      int cc = c - w; /* for RAT, we subtract the edge value. */
      int ce = RAT[e];
      if (ce > cc) {
        /* Critical path */
        RAT[e] = cc;
      }
      indeg[e]--;
      if (indeg[e] == 0) {
        q[nq++] = e;
      }
    }
  }

  return true;
}

/*
 * From wikipedia: (Path-based strong component algorithm)
 *
 * When the depth-first search reaches a vertex v, the algorithm performs the
 * following steps:
 *
 * - Set the preorder number of v to C, and increment C.
 * - Push v onto S and also onto P.
 * - For each edge from v to a neighboring vertex w:
 *     -  If the preorder number of w has not yet been assigned (the edge is a
 *        tree edge), recursively search w;
 *     -  Otherwise, if w has not yet been assigned to a strongly connected
 *        component (the edge is a forward/back/cross edge):
 *        - Repeatedly pop vertices from P until the top element of P has a
 *          preorder number less than or equal to the preorder number of w.
 * - If v is the top element of P:
 *    - Pop vertices from S until v has been popped, and assign the popped
 *      vertices to a new component.
 *    - Pop v from P.
 */
int strongly_connected_components(GraphL* g, int* comp, int* compsz) {
  typedef struct {
    int v;
    int i;
    int off;
    int ne;
  } visit_stack_t;
  int nv = g->nv;                                        /* num nodes */
  int C = 0;                                             /* C counter */
  int* ne_off = g->ne_off;                               /* graph edge offset */
  int* pre = malloc(nv * sizeof(int));                   /* preorder number */
  int* S = malloc(nv * sizeof(int));                     /* S stack*/
  int* P = malloc(nv * sizeof(int));                     /* P stack */
  visit_stack_t* vs = calloc(nv, sizeof(visit_stack_t)); /* DFS stack*/

  int nS = 0;  /* S Stack size */
  int nP = 0;  /* P Stack size */
  int nc = 0;  /* Number of SCC's found. */
  int nvs = 0; /* Size of the DFS state stack */
  int vi = 0;  /* Vertex being visited first (root of the DFS) */
  for (int i = 0; i < nv; i++) {
    pre[i] = -1;
    comp[i] = -1;
    compsz[i] = 0;
  }

  while (true) {
    if (nvs == 0) {
      /* No DFS stack, need to find the next non-visited node and start from
       * there. */
      while (vi < nv) {
        /* if vi is not visited*/
        if (pre[vi] < 0) {
          break;
        }
        vi++;
      }
      /* no next node found, stops algorithm */
      if (vi == nv) {
        break;
      }
      int off = ne_off[vi];
      int ne = ne_off[vi + 1] - off;
      vs[nvs++] = (visit_stack_t){
          vi,
          0,
          off,
          ne,
      };
    }
    /* top of stack */
    visit_stack_t* stk = &vs[nvs - 1];

    /* Entering node v (index i=0) */
    if (stk->i == 0) {
      /* 1. Set the preorder number of v to C, and increment C. */
      pre[stk->v] = C++;
      /* 2. Push v onto S and also onto P.  */
      S[nS++] = stk->v;
      P[nP++] = stk->v;
    }

    /* if i < ne, means it hasn't visited all its neighbours */
    if (stk->i < stk->ne) {
      int w = g->edges[stk->off + stk->i].e; /* neighbour w */
      stk->i++;
      /* If the preorder number of w has not yet been assigned */
      if (pre[w] == -1) {
        /* recursively search w */
        int off = ne_off[w];
        int ne = ne_off[w + 1] - off;
        vs[nvs++] = (visit_stack_t){
            w,
            0,
            off,
            ne,
        };
      } else {
        /* Otherwise, if w has not yet been assigned to a strongly connected
         * component */
        if (comp[w] == -1) {
          /* Repeatedly pop vertices from P until the top element of P has a
           * preorder number less than or equal to the preorder number of w.
           * */
          while (pre[P[nP - 1]] > pre[w]) nP--;
        }
      }
    } else {
      /* Pop() part:  */
      /* If v is the top element of P: */
      if (stk->v == P[nP - 1]) {
        /* Pop vertices from S until v has been popped, and assign the popped
         * vertices to a new component. */
        while (S[nS - 1] != stk->v) {
          comp[S[nS - 1]] = nc;
          compsz[nc]++;
          nS--;
        }
        comp[S[nS - 1]] = nc;
        compsz[nc]++;
        nS--;
        nc++;
        nP--;
      }
      nvs--;
    }
  }

#if 0
  for (int i = 0; i < nv; i++) {
    printf("SCC[%d] = %d\n", i, comp[i]);
  }

  for (int i = 0; i < nv; i++) {
    int sz = compsz[i];
    printf("comp_size[%d] = %d\n", i, compsz[i]);
  }
#endif

  free(vs);
  free(pre);
  free(S);
  free(P);
  return nc;
}

