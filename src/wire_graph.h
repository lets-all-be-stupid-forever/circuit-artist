#ifndef CA_WIRE_GRAPH_H
#define CA_WIRE_GRAPH_H

#include "pixel_graph.h"

// Error flags for circuit parsing
enum {
  STATUS_OK = 0,
  STATUS_DISCONNECTED = 1,
  STATUS_CONFLICT = 4,
  STATUS_TOOSLOW = 8,
};

typedef struct {
  int socket;   /* SocketID */
  int dt_ticks; /* Socket Delay (filled in dist_graph) */
} SocketDesc;

typedef struct {
  int nwire;               /* num of wires */
  int* comp;               /* wire number of each graph node */
  int* wmap[MAX_LAYERS];   /* pixel->wire map Has same size as image */
  int* wire_to_drv;        /* wireId --> driverId correspondence */
  int* drv_to_wire;        /* driverId --> wireId correspondence */
  int* skt_to_wire;        /* socketId --> wireId correspondence */
  int* drv_status;         /* error status of each driver */
  int* skt_status;         /* error status of each socket */
  bool has_errors;         /* errors during parsing */
  SocketDesc* wire_to_skt; /* Sockets at each wire */
  int* wire_to_skt_off;    /* Offset for the sockets at each wire */
  int global_error_flags;  /* Flag with each error type (during parsing) */
} WireGraph;

void wire_graph_init(WireGraph* wg, int nl, int w, int h, PixelGraph* pg,
                     bool debug);

void wire_graph_destroy(WireGraph* w);

#endif
