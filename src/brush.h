#ifndef BRUSH_H
#define BRUSH_H
#include "img.h"

// Context for 2d Brush tool.
//
// A brush context is basically a concatenated list of 2D points that is
// created while the user drags his mouse with cursor down.
typedef struct {
  // Number of points in the current path.
  int path_cnt;
  // Maximum buffer size for points. If the limit is reached, the buffer is
  // re-allocated and capacity is increased.
  int max_path_size;
  // List of XY pixel points in the brush path.
  // It's stored like X0,Y0,X1,Y1,X2,Y1, ...
  int* path_pixels;
} Brush;

// Initializes brush algorithm context.
void BrushLoad(Brush* b);

// Destructor.
void BrushUnload(Brush* b);

// Adds a new point to the brush path.
void BrushAppendPoint(Brush* b, int px, int py);

// Resets the brush path.
void BrushReset(Brush* b);

// Transforms the path into a Image, using a slightly modified version of
// bresenham line algorithm, where we make sure that the path is connected via
// 4-neighbour connectivity (so that it belongs to the same wire).
//
// It takes as argument the color of the brush (`c`), the total size of the
// original image (`w` and `h`), a pointer to an image that is created and a
// pointer to the offset of the created image.
// The created image is actually a sub-image containing the path, it's not a
// image of the size of the original image. This sub-image has an offset
// returned via the `off` parameter.
//
// The caller should take ownership of the created `out` image.
void BrushMakeImage(Brush* b, Color c, int w, int h, Image* out, Vector2Int* off);

#endif
