#ifndef CA_ELMORE_H
#define CA_ELMORE_H
#include "common.h"
#include "graph.h"

typedef struct {
  int v;
  float w;
} ParentEdge;

/* Computes Elmore Delay at a given RC Tree.
 *
 * Uses pi-model for capacitance distribution.
 * Each tree edge contains a length w. The capacitance per length is c_per_w and
 * resitance per length is r_per_w, constant across the tree.
 *
 * The output is the total capacitance at the root node and the delay at each
 * node. (e->delay array)
 */
typedef struct {
  DistSpec phys;
  int cap;            /* capacity (in nodes) of buffers */
  float* cdown;       /* temporary buffer for downstream capacitance */
  int* sorted;        /* Temporary buffer for topologically sorted nodes */
  ParentEdge* parent; /* Temporary buffer */
  float ctotal;       /* Total capacitance in the last computed tree */
} ElmoreCalculator;

ElmoreCalculator* elmore_calculator_create();
void elmore_calculator_free(ElmoreCalculator* e);
void elmore_calculator_run(ElmoreCalculator* e, int n, int me, int* ecount,
                           int* layer, GraphEdge* edges, int root,
                           float* node_distance);

#endif
