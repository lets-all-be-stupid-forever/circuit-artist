#ifndef CA_PIXEL_GRAPH_H
#define CA_PIXEL_GRAPH_H

// #include "dist_graph.h"
#include "graph.h"
#include "nand_detection.h"
#include "pin_spec.h"

typedef struct {
  Graph g;             /* 3D pixel node graph */
  NandLoc* nands;      /* STB array of nands in image. */
  int* skt;            /* Pixel Index of each socket. stb array */
  int* drv;            /* Pixel Index of each driver. stb array */
  u8* ori[MAX_LAYERS]; /* Orientation of a pixel. 0 is horizontal, 1 is
                          vertical. */
  int* pgoff;          /* socket/driver offset of each pingroup */
} PixelGraph;

void pixel_graph_init(PixelGraph* pg, DistSpec spec, int nl, Image* imgs,
                      PinGroup* p, bool debug);
void pixel_graph_destroy(PixelGraph* pg);

#endif
