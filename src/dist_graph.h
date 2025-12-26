#ifndef CA_DISTANCE_CALCULATION_H
#define CA_DISTANCE_CALCULATION_H

#include "common.h"
#include "graph.h"
#include "wire_graph.h"

typedef struct {
  float max_delay;    /* Maximum time delay in the wire */
  float pulse_energy; /* Energy spent in wire pulse */
} WireProps;

typedef struct {
  int wire_id;   /* Wire Id */
  int idx_start; /* First index */
  int idx_end;   /* Last index */
} WireSegment;

typedef struct {
  WireProps* wprop; /* max distance of each wire (in time steps) */
  int* gate_delay;  /* activation delay of each WIRE (when gate is present) */
  float* distmap[MAX_LAYERS];       /* distance for each pixel */
  WireSegment* seglist[MAX_LAYERS]; /* Array of segments, ordered by wireId */

  double t_setup;
  double t_build;
  double t_elmore;
} DistGraph;

void dist_graph_init(DistGraph* dg, DistSpec spec, int w, int h, int nl,
                     Graph* g, int* wire_to_drv, /* driver ID for each wire */
                     int* p_drv,              /* image index for each driver */
                     SocketDesc* wire_to_skt, /* Sockets at each wire */
                     int* wire_to_skt_off,    /* offset for each wire */
                     int* p_skt,              /* image index of each socket */
                     int n_skt,               /* Number of sockets */
                     int nc,                  /* number of components/wires */
                     int* comp,               /* WireId of each (graph) node */
                     u8** ori,                /* Orientation of each pixel */
                     bool debug);
void dist_graph_destroy(DistGraph* dg);

#endif
