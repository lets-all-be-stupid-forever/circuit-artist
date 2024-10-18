#include "brush.h"

#include <assert.h>
#include <stdlib.h>

void BrushLoad(Brush* b) {
  *b = (Brush){0};
  b->max_path_size = 1000;
  b->path_cnt = 0;
  b->path_pixels = calloc(b->max_path_size, sizeof(int));
}

void BrushAppendPoint(Brush* b, int px, int py) {
  // If have reached maximum size for brush, dont add more points.
  // (otherwise we might have issues in drawing as well)
  if (2 * b->path_cnt == b->max_path_size) {
    return;
  }
  // Won't add point to brush if it's the same as last one.
  int c = b->path_cnt - 1;
  if (c >= 0 && b->path_pixels[2 * c + 0] == px &&
      b->path_pixels[2 * c + 1] == py) {
    return;
  }

  b->path_pixels[2 * b->path_cnt + 0] = px;
  b->path_pixels[2 * b->path_cnt + 1] = py;
  b->path_cnt++;
}

void BrushReset(Brush* b) { b->path_cnt = 0; }

static void BrushMakePixels(Brush* b, int w, int h, int* npts, int** pts) {
  int size = 10000;
  int n = 0;
  int xmin = 0;
  int ymin = 0;
  int xmax = w;
  int ymax = h;
  int* out = malloc(size * sizeof(int));
  for (int i = 0; i < b->path_cnt; i++) {
    int px = b->path_pixels[2 * i + 0];
    int py = b->path_pixels[2 * i + 1];
    if (2 * n + 2 >= size) {
      size = size * 2;
      out = realloc(out, size * sizeof(int));
    }
    if (i == 0) {
      out[2 * n + 0] = px;
      out[2 * n + 1] = py;
      n = n + 1;
    } else {
      int x1 = px;
      int y1 = py;
      int x0 = b->path_pixels[2 * (i - 1) + 0];
      int y0 = b->path_pixels[2 * (i - 1) + 1];
      if (x1 != x0 || y1 != y0) {  // New bresenham:  we don't want diagonals
        // Breseham line from (px0, py0) to (px1, py1)
        // from https://gist.github.com/bert/1085538
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2; /* error value e_xy */
        for (;;) {
          // setPixel(x0, y0);
          if (2 * n + 2 >= size) {
            size = size * 2;
            out = realloc(out, size * sizeof(int));
          }
          out[2 * n + 0] = x0;
          out[2 * n + 1] = y0;
          n = n + 1;
          if (x0 == x1 && y0 == y1) break;
          e2 = 2 * err;
#if 0  // this commented version here is the original bresenham algorithm
          if (e2 >= dy) {
            err += dy;
            x0 += sx;
          } /* e_xy+e_x > 0 */
          if (e2 <= dx) {
            err += dx;
            y0 += sy;
          } /* e_xy+e_y < 0 */
#else  // version without diagonals
       // https://stackoverflow.com/questions/8936183/bresenham-lines-w-o-diagonal-movement/28786538#28786538
          if (e2 - dy > dx - e2) {
            err += dy;
            x0 += sx;
          } /* e_xy+e_x > 0 */
          else {
            err += dx;
            y0 += sy;
          } /* e_xy+e_y < 0 */
#endif
        }
      }
    }
  }
  int k = 0;
  for (int i = 0; i < n; i++) {
    int x = out[2 * i + 0];
    int y = out[2 * i + 1];
    if (x >= xmin && x < xmax && y >= ymin && y < ymax) {
      out[2 * k + 0] = x;
      out[2 * k + 1] = y;
      k = k + 1;
    }
  }
  if (k == 0) {
    free(out);
    *pts = NULL;
    *npts = 0;
    return;
  } else {
    *pts = out;
    *npts = k;
    return;
  }
}

void BrushMakeImage(Brush* b, Color c, int wtgt, int htgt, Image* out,
                    Vector2Int* off) {
  int* pts;
  int n;
  BrushMakePixels(b, wtgt, htgt, &n, &pts);
  if (n == 0) {
    *out = (Image){0};
    *off = (Vector2Int){0};
    return;
  }

  int xmin = pts[0];
  int xmax = pts[0];
  int ymin = pts[1];
  int ymax = pts[1];
  for (int i = 1; i < n; i++) {
    int x = pts[2 * i + 0];
    int y = pts[2 * i + 1];
    xmin = xmin < x ? xmin : x;
    ymin = ymin < y ? ymin : y;
    xmax = xmax > x ? xmax : x;
    ymax = ymax > y ? ymax : y;
  }

  int w = xmax - xmin + 1;
  int h = ymax - ymin + 1;

  off->x = xmin;
  off->y = ymin;
  Image img = GenImageFilled(w, h, BLANK);
  *out = img;
  Color* colors = GetPixels(img);
  for (int i = 0; i < n; i++) {
    int x = pts[2 * i + 0] - xmin;
    int y = pts[2 * i + 1] - ymin;
    assert(x >= 0 && x < w);
    assert(y >= 0 && y < h);
    colors[y * w + x] = c;
  }
  free(pts);
}

void BrushUnload(Brush* b) { free(b->path_pixels); }
