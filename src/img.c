#include "img.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "math.h"
#include "raymath.h"
#include "rectint.h"
#include "rlgl.h"
#include "shaders.h"

#define LAYER_MAGIC_1 7
#define LAYER_MAGIC_2 11
#define LAYER_MAGIC_3 17

#if 0
#if defined(GRAPHICS_API_OPENGL_33)
#include "external/glad.h"  // This should be included by rlgl
#elif defined(GRAPHICS_API_OPENGL_ES2)
// For OpenGL ES 2.0
#endif
#endif

// Makes sure the alpha is 255
static Color to_valid_color(Color c) { return (Color){c.r, c.g, c.b, 255}; }

// From RGB color to 0-255 gray value.
static inline int color2gray(Color c) {
  float r = c.r;
  float g = c.g;
  float b = c.b;
  int out = 0.299f * r + 0.587f * g + 0.114f * b;
  if (out < 0) out = 0;
  if (out > 255) out = 255;
  return out;
}

static bool is_rect_inside(Image img, RectangleInt region) {
  RectangleInt intersection =
      rect_int_get_collision(get_image_rect(img), region);
  bool ok = true;
  ok = ok && (intersection.x == region.x);
  ok = ok && (intersection.y == region.y);
  ok = ok && (intersection.height == region.height);
  ok = ok && (intersection.width == region.width);
  return ok;
}

// Returns a pointer to the casted image data.
// It will crash if the image format is not R8G8B8A8.
Color* get_pixels(Image img) {
  if (img.width == 0 || img.height == 0) {
    return NULL;
  }
  assert(img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  return (Color*)img.data;
}

// It's just to make it easier to compare colors
int* get_pixels_as_int(Image img) {
  assert(img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  return (int*)img.data;
}

// Returns the RectangleInt containing the whole image.
RectangleInt get_image_rect(Image img) {
  return (RectangleInt){0, 0, img.width, img.height};
}

// Thin wrapper over GenImage of raylib. The idea is that eventually the images
// will be forced to be in a specific format (like a 128-sized palette or
// something.)
Image gen_image_simple(int w, int h) {
  // Keeping this function because eventually we might want to enforce the
  // default image data type (like keeping a palette).
  return gen_image_filled(w, h, BLACK);
}

// This wrapper over GenImageColor.
Image gen_image_filled(int w, int h, Color v) { return GenImageColor(w, h, v); }

// Duplicates an image.
Image clone_image(Image img) { return crop_image(img, get_image_rect(img)); }

// Fills image with a color.
void fill_image(Image* img, Color v) {
  int size = img->width * img->height;
  Color* color_data = get_pixels(*img);
  for (int i = 0; i < size; i++) {
    color_data[i] = v;
  }
}

// Crops an image. Returns a new image.
Image crop_image(Image img, RectangleInt region) {
  assert(is_rect_inside(
      img, region));  // Checks that the cropping region is inside the image
  assert(!rect_int_is_empty(region));  // cropped region shouldn't be empty
  int w = region.width;
  int h = region.height;
  Image cropped = gen_image_simple(w, h);
  Color* cropped_pixels = get_pixels(cropped);
  Color* img_pixels = get_pixels(img);
  for (int y = 0; y < h; y++) {
    int off_img = (y + region.y) * img.width + (region.x);
    int off_cropped = y * cropped.width;
    for (int x = 0; x < w; x++) {
      cropped_pixels[off_cropped + x] = img_pixels[off_img + x];
    }
  }
  return cropped;
}

// Rotates an image. Returns a new image.
Image rotate_image(Image img, int ccw) {
  Image out = gen_image_simple(img.height, img.width);
  Color* pout = get_pixels(out);
  Color* pin = get_pixels(img);
  for (int y = 0; y < img.height; y++) {
    for (int x = 0; x < img.width; x++) {
      if (ccw == 1) {
        int pix_out = (img.width - x - 1) * out.width + y;
        int pix_in = y * img.width + x;
        pout[pix_out] = pin[pix_in];
      } else {
        int pix_in = y * img.width + x;
        int pix_out = x * out.width + (img.height - y - 1);
        pout[pix_out] = pin[pix_in];
      }
    }
  }
  return out;
}

//  Flips an image horizontally inplace.
void flip_image_h_inplace(Image* img) {
  Color* colors = get_pixels(*img);
  for (int y = 0; y < img->height; y++) {
    for (int x = 0; x < img->width / 2; x++) {
      int p1 = y * img->width + x;
      int p2 = y * img->width + (img->width - x - 1);
      Color tmp = colors[p1];
      colors[p1] = colors[p2];
      colors[p2] = tmp;
    }
  }
}

//  Flips an image vertically inplace.
void flip_image_v_inplace(Image* img) {
  Color* colors = get_pixels(*img);
  for (int y = 0; y < img->height / 2; y++) {
    for (int x = 0; x < img->width; x++) {
      int p1 = y * img->width + x;
      int p2 = (img->height - y - 1) * img->width + x;
      Color tmp = colors[p1];
      colors[p1] = colors[p2];
      colors[p2] = tmp;
    }
  }
}

// Fills a subset of an image with a color.
void fill_image_rect(Image* img, RectangleInt r, Color c) {
  assert(is_rect_inside(*img, r));
  int x0 = r.x;
  int y0 = r.y;
  int w = r.width;
  int h = r.height;
  int ww = img->width;
  Color* colors = get_pixels(*img);
  for (int y = y0; y < y0 + h; y++) {
    for (int x = x0; x < x0 + w; x++) {
      int p = y * ww + x;
      colors[p] = c;
    }
  }
}

// A special version of image concatenation that treats black pixels as blank
// pixels (full transparency). Also makes sure there's no actual black in the
// image but BLANKS.
void image_combine(Image src, RectangleInt r, Image* dst, Vector2Int offset) {
  assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= src.width &&
         r.y + r.height <= src.height);
  assert(offset.x >= 0 && offset.y >= 0 && offset.x + r.width <= dst->width &&
         offset.y + r.height <= dst->height);
  Color* csrc = get_pixels(src);
  Color* cdst = get_pixels(*dst);
  int w = r.width;
  int h = r.height;
  int ox1 = r.x;
  int oy1 = r.y;
  int ox2 = offset.x;
  int oy2 = offset.y;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int y1 = oy1 + y;
      int x1 = ox1 + x;
      int y2 = oy2 + y;
      int x2 = ox2 + x;
      int p1 = y1 * src.width + x1;
      int p2 = y2 * dst->width + x2;
      if (COLOR_EQ(csrc[p1], BLACK)) {
        // When it's black, I want to keep blank in the dest image
        cdst[p2] = BLANK;
      } else if (!COLOR_EQ(csrc[p1], BLANK)) {
        cdst[p2] = csrc[p1];
      }
    }
  }
}

// Copies a subset of an image in another image.
void copy_image(Image src, RectangleInt r, Image* dst, Vector2Int offset) {
  assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= src.width &&
         r.y + r.height <= src.height);
  assert(offset.x >= 0 && offset.y >= 0 && offset.x + r.width <= dst->width &&
         offset.y + r.height <= dst->height);
  Color* csrc = get_pixels(src);
  Color* cdst = get_pixels(*dst);
  int w = r.width;
  int h = r.height;
  int ox1 = r.x;
  int oy1 = r.y;
  int ox2 = offset.x;
  int oy2 = offset.y;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int y1 = oy1 + y;
      int x1 = ox1 + x;
      int y2 = oy2 + y;
      int x2 = ox2 + x;
      int p1 = y1 * src.width + x1;
      int p2 = y2 * dst->width + x2;
      if (COLOR_EQ(csrc[p1], BLACK)) {
        cdst[p2] = BLANK;
      } else {
        cdst[p2] = csrc[p1];
      }
    }
  }
}

// Replaces Blacks in images by BLANKs inplace.
// Also remove weird transparent pixels.
// Usually called when an image is imported.
void image_remove_blacks(Image* img) {
  Color* colors = get_pixels(*img);
  for (int y = 0; y < img->height; y++) {
    for (int x = 0; x < img->width; x++) {
      int p = y * img->width + x;
      int c = color2gray(colors[p]);
      if (c < 1) {
        colors[p] = BLANK;
      } else {
        colors[p] = to_valid_color(colors[p]);
      }
    }
  }
}

// Replaces BLANKs in an image by BLACKs inplace.
// Usually called when an image is exported.
static void image_add_blacks(Image img) {
  Color* colors = get_pixels(img);
  for (int y = 0; y < img.height; y++) {
    for (int x = 0; x < img.width; x++) {
      int p = y * img.width + x;
      int c = color2gray(colors[p]);
      if (c > 1) {
        colors[p] = to_valid_color(colors[p]);
      } else {
        colors[p] = BLACK;
      }
    }
  }
}

// Ensures an image is smaller than a given size. (defined by a macro)
void image_ensure_max_size(Image* img, int max_size) {
  if (img->width > max_size || img->height > max_size) {
    int w = img->width;
    int h = img->height;
    int w2 = (w > max_size) ? max_size : w;
    int h2 = (h > max_size) ? max_size : h;
    Image smaller_img = gen_image_simple(w2, h2);
    Color* p1 = get_pixels(*img);
    Color* p2 = get_pixels(smaller_img);
    for (int y = 0; y < h2; y++) {
      for (int x = 0; x < w2; x++) {
        p2[y * w2 + x] = p1[y * w + x];
      }
    }
    UnloadImage(*img);
    *img = smaller_img;
  }
}

// Algorithm for drawing the "Line Tool".
//
// It draws straight lines with the caveats:
// (i) it can draw multiple parallel lines at the same time via the "ls" (or
// line size) parameter.
// (ii) when drawing multiple lines, it can also adjust beginning and end tips
// of the lines so that it makes corners (when user presses shift/ctrl). This
// feature is useful for creating wire corners or connecting a list of wires at
// the same time.
//
// It creates a new image `out` which becomes ownership of the caller.
//
// The `start` parameter is where  the user first clicked during the use of the
// tool. It is used to define where the line "starts" from. The `tool_rect`
// parameter contains the rectangle generated by starting and end mouse points.
//
// `corner` is a flag for the corner at the beginning tip of the wire and
// `end_corner` is a flag for corenr at the end tip of the lines.
//
// `sep` is the separation between consecutive lines.
//
// The generated image `out` is a subset of the full image . The full image
// size is defined by `img_rect`,  and the offset of the subimage within the
// full image is given by the `off` output.
void draw_image_line_tool(Vector2Int start, RectangleInt tool_rect,
                          RectangleInt img_rect, int ls, int sep, bool corner,
                          bool end_corner, Color c, Image* out,
                          Vector2Int* off) {
  const int DIR_HORIZONTAL = 0;
  const int DIR_VERTICAL = 1;
  RectangleInt r = tool_rect;
  int startx = start.x;
  int starty = start.y;
  /* end is the index opposite side of the tool rectangle
   * A + B = x + (x+w-1).
   * C = A+B -D = (2x+w-1) - D
   * */
  int endx = (2 * r.x + r.width - 1) - startx;
  int endy = (2 * r.y + r.height - 1) - starty;
  // printf("starty/endy: %d %d \n", starty, endy);

  /* Finds direction of lines, depending on weither the tool rectangle is wider
   * or taller.( wider = x direction, taller = y direction)*/
  int dir = -1;
  if (r.width >= r.height) {
    /* Wider */
    dir = DIR_HORIZONTAL;
    /* Overrides tool rectangle with actual final rectangle taking into account
     * number of lines. */
    r = (RectangleInt){
        .x = r.x,    /* X starts at left corner of rectangle */
        .y = starty, /* Y starts at same position as where user has clicked */
        .width = r.width, /* Same width as original tool rectangle */
        .height = (sep + 1) * ls - sep, /* height will depend in functino of
                                         number of lines and line separation */
    };
  } else {
    /* Taller */
    dir = DIR_VERTICAL;
    r = (RectangleInt){
        .x = startx,
        .y = r.y,
        .width = (sep + 1) * ls - sep,
        .height = r.height,
    };
  }

  int w = r.width;
  int h = r.height;
  int hh = 2 * ls - 1;
  int ww = 2 * ls - 1;
  Image img = gen_image_filled(w, h, BLANK);
  *out = img;
  // horizontal
  Color* data = get_pixels(*out);
  if (dir == DIR_HORIZONTAL) {
    for (int yy = 0, y = 0; yy < img.height; yy += sep + 1, y += 2) {
      Color* line = data + yy * img.width;
      for (int x = 0; x < img.width; x++) {
        // Horizontal lines
        // When end_corner is true, it's like it starts from the other end.
        if (corner) {
          int dx = x + r.x - startx;
          int dy = y + r.y - starty;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (endx > startx && dx < dy) continue;
          if (endx < startx && dx < (hh - dy - 1)) continue;
        }
        if (end_corner) {
          // Just exchange startx and endx
          int dx = x + r.x - endx;
          int dy = y + r.y - starty;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (startx > endx && dx < dy) continue;
          if (startx < endx && dx < (hh - dy - 1)) continue;
        }
        line[x] = c;
      }
    }
  } else {
    for (int y = 0; y < img.height; y++) {
      Color* line = data + y * img.width;
      for (int xx = 0, x = 0; xx < img.width; xx += sep + 1, x += 2) {
        if (corner) {
          int dx = x + r.x - startx;
          int dy = y + r.y - starty;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (endy > starty && dx > dy) continue;
          if (endy < starty && (ww - dx - 1) > dy) continue;
        }
        if (end_corner) {
          int dx = x + r.x - startx;
          int dy = y + r.y - endy;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (starty > endy && dx > dy) continue;
          if (starty < endy && (ww - dx - 1) > dy) continue;
        }
        line[xx] = c;
      }
    }
  }
  // Now I adjust offset to make sure the mouse is in the right side of the
  // line drawn
  if (dir == DIR_HORIZONTAL) {
    //
    //    If cursor is here, we want to shift the rectagnle up.
    // ---X------->
    //
    if (endy < starty) {
      r.y -= r.height - 1;
    }
  }
  if (dir == DIR_VERTICAL) {
    if (endx < startx) {
      r.x -= r.width - 1;
    }
  }
  off->x = r.x;
  off->y = r.y;
}

static inline int get_crossing_pixel_direction(int* pixels, int w, int h, int x,
                                               int y) {
  if (x == 0 || x == w - 1 || y == 0 || y == h - 1) {
    return false;
  }
  int idx = y * w + x;
  int c = pixels[idx];
  int nh = 0;
  int nv = 0;
  nv += pixels[idx - w] == c;
  nv += pixels[idx + w] == c;
  nh += pixels[idx - 1] == c;
  nh += pixels[idx + 1] == c;
  if (nv > nh) {
    return 1;
  } else {
    return 0;
  }
}

static inline bool is_crossing(int* pixels, int w, int h, int x, int y) {
  if (x == 0 || x == w - 1 || y == 0 || y == h - 1) {
    return false;
  }
  int idx = y * w + x;
  if (pixels[idx - 1] == 0) return false;
  if (pixels[idx + 1] == 0) return false;
  if (pixels[idx - w] == 0) return false;
  if (pixels[idx + w] == 0) return false;
  return true;
}

// Algorithm for the "Bucket Tool".
//
// `img` is the original image before the tool is applied.
// `x`, `y`, `sw` and `sh` define the region where the bucket should be applied
// to with the color `c`.
//
// The `out` image contains a sub-image of the original image (positined by
// `off` offset) containing the new pixels after modification. The final result
// is the combination of the initial image with this new image.
//
// Note that it doesnt modify the image inplace directly.
//
// It's up to the caller to take ownership of the generated `out` image.
void draw_image_bucket_tool(Image img, int x, int y, int sw, int sh, Color c,
                            bool force, Image* out, Vector2Int* off) {
  // Idea: Like a floodfill but only wire logic.
  // A bit tricky. Need to identify how it would be displayed in the
  // simulation.

  bool erasing = c.r == 0 && c.g == 0 && c.b == 0;
  // Each pixel is either:
  // - a crossing
  // - a line
  // - a T
  // Maybe I can do as in the simu parsing, at each time compute only the
  // vertical or horizontal components
  //
  // Line sections = lines that extend until finding black. (vertical or
  // horizontal)
  //
  // Each pixel has it's horizontal and vertical line passing through.
  // They always intersect, **with exception of crossings**
  //
  //
  // Algorithm:
  // COMP_X[x,y] = 0
  // COMP_Y[x,y] = 0
  // Q = []
  // XMIN,YMIN=x,y
  // XMAX,YMAX=x,y
  //
  // if IsHorizontalCrossingPixel(img, x, y)
  //    COMP_X[x,y] = 1
  //    QueueAdd(Q, (x, y, H))
  // else
  //    COMP_Y[x, y] = 1
  //    QueueAdd(Q, (x, y, V))
  // end
  //
  //  out = makesubimage(xmax-xmin, ymax-ymin)
  //  for each pixel in subregion:
  //      n = compx + compy
  //      if n is 2: out is C (color given as input)
  //      if n is 1:
  //        if IsHorizontalCrossing()
  //            if compx is 1: out is C
  //         else
  //            if compy is 1: out is C
  //
  int w = img.width;
  int h = img.height;
  Color* pixels_clr = img.data;
  int* pixels = get_pixels_as_int(img);
  int idx = y * w + x;
  if (w == 1 && h == 1 && pixels[idx] == 0) {
    *out = (Image){0};
    *off = (Vector2Int){0};
    return;
  }
  char* draft = malloc(2 * w * h * sizeof(char));
  int qsize = 10000;
  int* Q = malloc(qsize * sizeof(int));
  int qtop = 0;
  int xmin = x;
  int xmax = x;
  int ymin = y;
  int ymax = y;
  for (int i = 0; i < w * h; i++) {
    draft[2 * i + 0] = 0;
    draft[2 * i + 1] = 0;
  }

  for (int yy = y; yy < y + sh; yy++) {
    for (int xx = x; xx < x + sw; xx++) {
      int idx = yy * w + xx;
      // I don't want to paint black pixels.
      if (pixels[idx] == 0) {
        continue;
      }
      int d = get_crossing_pixel_direction(pixels, w, h, xx, yy);
      Q[qtop++] = 2 * idx + d;
      if (qtop == qsize) {
        qsize = 2 * qsize;
        Q = realloc(Q, qsize * sizeof(int));
      }
    }
  }

  if (qtop == 0) {
    // Nothing to do here.
    free(Q);
    free(draft);
    return;
  }

  while (qtop > 0) {
    int nxt = Q[--qtop];
    int cy = (nxt / 2) / w;
    int cx = (nxt / 2) % w;
    ymin = cy < ymin ? cy : ymin;
    ymax = cy > ymax ? cy : ymax;
    xmin = cx < xmin ? cx : xmin;
    xmax = cx > xmax ? cx : xmax;

    if (draft[nxt] == 1) {
      continue;
    }
    int t = nxt & 1;
    int dx = 1 - t;
    int dy = t;

    int xx = cx;
    int yy = cy;
    /*
     * while not QueueEmpty(Q)
     *   x, y, D = QueuePop(Q)
     *   if COMP[D][x, y] == 1 then
     *       continue
     *   end
     *  // find line boundaries()
     *   For each pixel p in LINE(x, y, D) do
     *       COMP(D,x,y) = 1
     *       update_xmin_xmax(x, y)
     *       if COMPY(x, y) == 0 then
     *           if not is_crossing(x, y) then
     *               QueueAdd(Q, (1-D, px, py) )
     *           end
     *       end
     *   end
     * end
     */
    while (xx < w && yy < h) {
      int cidx = yy * w + xx;
      if (pixels[cidx] == 0) {
        break;
      }
      draft[2 * cidx + t] = 1;
      if (draft[2 * cidx + 1 - t] == 0) {
        if (!is_crossing(pixels, w, h, xx, yy)) {
          Q[qtop++] = 2 * cidx + 1 - t;
          if (qtop == qsize) {
            qsize *= 2;
            Q = realloc(Q, qsize * sizeof(int));
          }
        }
      }
      xx += dx;
      yy += dy;
    }
    xx = cx - dx;
    yy = cy - dy;
    while (xx >= 0 && yy >= 0) {
      int cidx = yy * w + xx;
      if (pixels[cidx] == 0) {
        break;
      }
      draft[2 * cidx + t] = 1;
      if (draft[2 * cidx + 1 - t] == 0) {
        if (!is_crossing(pixels, w, h, xx, yy)) {
          Q[qtop++] = 2 * cidx + 1 - t;
          if (qtop == qsize) {
            qsize *= 2;
            Q = realloc(Q, qsize * sizeof(int));
          }
        }
      }
      xx -= dx;
      yy -= dy;
    }
  }

  /*
   *  out = makesubimage(xmax-xmin, ymax-ymin)
   *  for each pixel in subregion:
   *      n = compx + compy
   *      if n is 2: out is C (color given as input)
   *      if n is 1:
   *        if IsHorizontalCrossing()
   *            if compx is 1: out is C
   *         else
   *            if compy is 1: out is C
   */
  off->x = xmin;
  off->y = ymin;
  int ww = xmax - xmin + 1;
  int hh = ymax - ymin + 1;
  Image rimg = gen_image_filled(ww, hh, BLANK);
  *out = rimg;
  Color* out_pixels = get_pixels(rimg);
  for (int iy = 0; iy < hh; iy++) {
    for (int ix = 0; ix < ww; ix++) {
      int idx = (iy + ymin) * w + (ix + xmin);
      int ih = draft[2 * idx + 0];
      int iv = draft[2 * idx + 1];
      int n = ih + iv;
      if (n == 0) continue;
      int oidx = iy * ww + ix;
      if (n == 2 || force) {
        out_pixels[oidx] = c;
      } else {
        if (!erasing) {
          if (get_crossing_pixel_direction(pixels, w, h, ix + xmin,
                                           iy + ymin)) {
            if (draft[2 * idx + 1]) out_pixels[oidx] = c;
          } else {
            if (draft[2 * idx + 0]) out_pixels[oidx] = c;
          }
        } else {
          int xx = ix + xmin;
          int yy = iy + ymin;
          int idx2 = yy * w + xx;
          if (get_crossing_pixel_direction(pixels, w, h, xx, yy)) {
            if (draft[2 * idx + 1]) out_pixels[oidx] = pixels_clr[idx2 - 1];
          } else {
            if (draft[2 * idx + 0]) out_pixels[oidx] = pixels_clr[idx2 - w];
          }
        }
      }
    }
  }
  free(Q);
  free(draft);
}

// Draws a rectangle in an image.
void draw_image_rect_simple(Image* img, int x, int y, int w, int h, Color c) {
  assert(is_rect_inside(*img, (RectangleInt){x, y, w, h}));
  int x0 = x;
  int y0 = y;
  int ww = img->width;
  Color* colors = get_pixels(*img);
  for (int y = y0; y < y0 + h; y++) {
    colors[y * ww + x0] = c;
    colors[y * ww + x0 + w - 1] = c;
  }
  for (int x = x0; x < x0 + w; x++) {
    colors[y0 * ww + x] = c;
    colors[(y0 + h - 1) * ww + x] = c;
  }
}

RenderTexture2D clone_texture(RenderTexture2D img) {
  RenderTexture2D out =
      LoadRenderTexture(img.texture.width, img.texture.height);
  BeginTextureMode(out);
  float tw = (float)img.texture.width;
  float th = (float)img.texture.height;
  Rectangle source = {0, 0, tw, -th};
  Rectangle target = {0, 0, tw, th};
  Vector2 pos = {0, 0};
  float rot = 0;
  DrawTexturePro(img.texture, source, target, pos, rot, WHITE);
  EndTextureMode();
  return out;
}

RenderTexture2D crop_texture(RenderTexture2D img, RectangleInt region) {
  int w = region.width;
  int h = region.height;
  RenderTexture2D out = LoadRenderTexture(w, h);
  BeginTextureMode(out);
  int th = img.texture.height;
  Rectangle source = {
      .x = (float)region.x,
      .y = (float)(th - region.y - region.height),
      .width = (float)region.width,
      .height = (float)-region.height,
  };
  Rectangle target = {
      0,
      0,
      (float)region.width,
      (float)region.height,
  };
  Vector2 position = {0, 0};
  DrawTexturePro(img.texture, source, target, position, 0, WHITE);
  EndTextureMode();
  return out;
}

RenderTexture2D clone_texture_from_image(Image img) {
  RenderTexture2D out = LoadRenderTexture(img.width, img.height);
  BeginTextureMode(out);
  ClearBackground(BLANK);
  Texture2D t = LoadTextureFromImage(img);
  DrawTexture(t, 0, 0, WHITE);
  //  Rectangle source = {0, 0, (float)img.width, (float)-img.height};
  //  Rectangle target = {0, 0, (float)img.width, (float)img.height};
  //  DrawTexturePro(t, source, target, (Vector2){0, 0}, 0, WHITE);
  EndTextureMode();
  UnloadTexture(t);
  return out;
}

void texture_combine(RenderTexture2D src, RectangleInt r, RenderTexture2D* dst,
                     Vector2Int offset) {
  assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= src.texture.width &&
         r.y + r.height <= src.texture.height);
  assert(offset.x >= 0 && offset.y >= 0 &&
         offset.x + r.width <= dst->texture.width &&
         offset.y + r.height <= dst->texture.height);
  BeginTextureMode(*dst);
  int th = src.texture.height;
  Rectangle source = {
      .x = (float)r.x,
      // Need to shift a bit here because the texture is inverted
      .y = (float)(th - (r.y + r.height)),
      .width = (float)r.width,
      .height = (float)-r.height,
  };
  Rectangle target = {
      .x = (float)offset.x,
      .y = (float)offset.y,
      .width = (float)r.width,
      .height = (float)r.height,
  };
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
  begin_shader(comb);
  int src_size[2] = {src.texture.width, src.texture.height};
  int dst_size[2] = {dst->texture.width, dst->texture.height};
  // int roi_size[2] = {r.width, r.height};
  int src_off[2] = {r.x, r.y};
  int dst_off[2] = {offset.x, offset.y};
  set_shader_tex(comb, dst_tex, dst->texture);
  set_shader_ivec2(comb, src_size, &src_size);
  set_shader_ivec2(comb, dst_size, &dst_size);
  // set_shader_ivec2(comb, roi_size, &roi_size);
  set_shader_ivec2(comb, src_off, &src_off);
  set_shader_ivec2(comb, dst_off, &dst_off);
  DrawTexturePro(src.texture, source, target, (Vector2){.x = 0, .y = 0}, 0.0f,
                 WHITE);
  end_shader();
  EndBlendMode();
  EndTextureMode();
}

void fill_texture_rect(RenderTexture* img, RectangleInt r, Color c) {
  BeginTextureMode(*img);
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
  begin_shader(fill);
  DrawRectangle(r.x, r.y, r.width, r.height, c);
  end_shader();
  EndBlendMode();
  EndTextureMode();
}

void copy_texture(RenderTexture2D src, RectangleInt r, RenderTexture2D* dst,
                  Vector2Int offset) {
  assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= src.texture.width &&
         r.y + r.height <= src.texture.height);
  // assert(offset.x >= 0 && offset.y >= 0 &&
  //        offset.x + r.width <= dst->texture.width &&
  //        offset.y + r.height <= dst->texture.height);
  BeginTextureMode(*dst);
  Rectangle source = {
      .x = (float)r.x,
      .y = (float)r.y,
      .width = (float)r.width,
      .height = (float)-r.height,
  };
  Rectangle target = {
      .x = (float)offset.x,
      .y = (float)offset.y,  // th - bottom,
      .width = (float)r.width,
      .height = (float)r.height,
  };
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
  DrawTexturePro(src.texture, source, target, (Vector2){.x = 0, .y = 0}, 0.0f,
                 WHITE);
  EndBlendMode();
  EndTextureMode();
}

void flip_texture_h_inplace(RenderTexture2D* img) {
  RenderTexture2D tmp = clone_texture(*img);
  BeginTextureMode(*img);
  float tw = (float)img->texture.width;
  float th = (float)img->texture.height;
  Rectangle source = {0, 0, -tw, -th};
  Rectangle target = {0, 0, tw, th};
  Vector2 pos = {0, 0};
  float rot = 0;
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
  DrawTexturePro(tmp.texture, source, target, pos, rot, WHITE);
  EndTextureMode();
  EndBlendMode();
  UnloadRenderTexture(tmp);
}

void flip_texture_v_inplace(RenderTexture2D* img) {
  RenderTexture2D tmp = clone_texture(*img);
  BeginTextureMode(*img);
  float tw = (float)img->texture.width;
  float th = (float)img->texture.height;
  Rectangle source = {0, 0, tw, th};
  Rectangle target = {0, 0, tw, th};
  Vector2 pos = {0, 0};
  float rot = 0;
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
  DrawTexturePro(tmp.texture, source, target, pos, rot, WHITE);
  EndBlendMode();
  EndTextureMode();
  UnloadRenderTexture(tmp);
}

RenderTexture2D rotate_texture(RenderTexture2D img, int ccw) {
  int tw = img.texture.width;
  int th = img.texture.height;
  RenderTexture2D out = LoadRenderTexture(th, tw);
  BeginTextureMode(out);
  Rectangle source = {0, 0, (float)tw, (float)-th};
  Rectangle target = {0, 0, (float)th, (float)tw};
  Vector2 pos = {0, 0};
  float rot = 0;
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  BeginBlendMode(BLEND_CUSTOM);
  begin_shader(rotate);
  int _ccw = ccw;
  set_shader_int(rotate, ccw, &_ccw);
  DrawTexturePro(img.texture, source, target, pos, rot, WHITE);
  end_shader();
  EndBlendMode();
  EndTextureMode();
  return out;
}

void draw_rt_on_screen(RenderTexture2D rt, Vector2 pos) {
  int tw = rt.texture.width;
  int th = rt.texture.height;
  Rectangle source = {
      .x = 0,
      .y = 0,
      .width = tw,
      .height = -th,
  };
  Rectangle target = {
      .x = pos.x,
      .y = pos.y,
      .width = tw,
      .height = th,
  };
  Vector2 v0 = {0};
  DrawTexturePro(rt.texture, source, target, v0, 0.0f, WHITE);
}

RenderTexture2D make_thumbnail(Image img, int tx, int ty) {
  RenderTexture2D tex = clone_texture_from_image(img);
  RenderTexture2D out = LoadRenderTexture(tx, ty);

  int tw = tex.texture.width;
  int th = tex.texture.height;
  float cs1 = ((float)tx) / tw;
  float cs2 = ((float)ty) / th;

  float cs = 1.0;
  if (cs1 < cs2) {
    cs = cs1;
  } else {
    cs = cs2;
  }

#if 0
  if (cs > 4.0) {
    cs = 4.0;
  } else if (cs > 2.0) {
    cs = 2.0;
  } else if (cs > 1.0) {
    cs = 1.0;
  } else if (cs > 0.5) {
    cs = 0.5;
  } else if (cs > 0.25) {
    cs = 0.25;
  } else {
    cs = 0.125;
  }
#endif

  float sp[2] = {cs, cs};
  int img_size[2] = {tw, th};

  BeginTextureMode(out);
  ClearBackground(BLANK);
  begin_shader(thumbnail);
  SetTextureFilter(tex.texture, TEXTURE_FILTER_POINT);
  set_shader_vec2(thumbnail, sp, &sp);
  set_shader_ivec2(thumbnail, img_size, &img_size);

  // Rectangle source = (Rectangle){0, (ty - (0 + img_size[1])), (float)mw,
  // (float)-mh};
  Rectangle source = (Rectangle){0, 0, (float)tw, (float)th};
  float mw = tw * cs;
  float mh = th * cs;
  float cx = (tx - mw) / 2;
  float cy = (ty - mh) / 2;
  Rectangle target = (Rectangle){cx, cy, mw, mh};
  DrawTexturePro(tex.texture, source, target, (Vector2){0, 0}, 0, WHITE);
  end_shader();
  EndTextureMode();
  UnloadRenderTexture(tex);
  return out;
}

static int arrowRight[] = {
    0, 0, 0, 1, 0, 0,  //
    0, 0, 0, 1, 1, 0,  //
    1, 1, 1, 1, 1, 1,  //
    0, 0, 0, 1, 1, 0,  //
    0, 0, 0, 1, 0, 0,  //
};

static int arrowLeft[] = {
    0, 0, 1, 0, 0, 0,  //
    0, 1, 1, 0, 0, 0,  //
    1, 1, 1, 1, 1, 1,  //
    0, 1, 1, 0, 0, 0,  //
    0, 0, 1, 0, 0, 0,  //
};

// Generates a 256x1 image with lookup table for HEATMAP lut.
Image gen_image_lut_heat() {
  Image out = GenImageColor(256, 1, BLACK);
  Color* colors = out.data;
  Color levels[5] = {
      {.r = 0, .g = 0, .b = 0, .a = 0},
      {.r = 139, .g = 0, .b = 0, .a = 64 - 1},
      {.r = 255, .g = 165, .b = 0, .a = 128 - 1},
      {.r = 255, .g = 255, .b = 0, .a = 192},
      {.r = 255, .g = 255, .b = 255, .a = 255},
  };

  for (int i = 0; i < 256; i++) {
    for (int j = 0; j < 4; j++) {
      if (i <= levels[j + 1].a) {
        float r0 = levels[j].r;
        float g0 = levels[j].g;
        float b0 = levels[j].b;
        float t0 = levels[j].a / 255.0;
        float r1 = levels[j + 1].r;
        float g1 = levels[j + 1].g;
        float b1 = levels[j + 1].b;
        float t1 = levels[j + 1].a / 255.0;
        float t = i / 255.0;
        float local_t = (t - t0) / (t1 - t0);
        local_t = local_t * local_t * (3.f - 2.f * local_t);
        int r = (int)(r0 + (r1 - r0) * local_t);
        int g = (int)(g0 + (g1 - g0) * local_t);
        int b = (int)(b0 + (b1 - b0) * local_t);
        r = r < 0 ? 0 : r;
        g = g < 0 ? 0 : g;
        b = b < 0 ? 0 : b;
        r = r > 255 ? 255 : r;
        g = g > 255 ? 255 : g;
        b = b > 255 ? 255 : b;
        colors[i].r = r;
        colors[i].g = g;
        colors[i].b = b;
        colors[i].a = 255;
        break;
      }
    }
  }

  return out;
}

void draw_projection_on_target(Cam2D cam, Tex2D tTmp, v2i szImg, int mode,
                               Color c) {
  begin_shader(project);
  Vector2 sp = {
      (float)cam.sp,
      (float)cam.sp,
  };
  int tmp_size[2] = {tTmp.width, tTmp.height};
  int img_size[2] = {szImg.x, szImg.y};
  SetTextureFilter(tTmp, TEXTURE_FILTER_POINT);
  // set_shader_int(project, mode, &mode);
  set_shader_vec2(project, sp, &sp);
  set_shader_ivec2(project, img_size, &tmp_size);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = cam.off.x;
  float cy = cam.off.y;
  float cs = cam.sp;
  DrawTexturePro(
      tTmp,
      (Rectangle){0, (tmp_size[1] - (0 + img_size[1])), (float)mw, (float)-mh},
      (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0, c);
  end_shader();
}

void draw_projection_on_target_pattern(Cam2D cam, Tex2D tex, v2i szImg,
                                       int pattern, int pad, Color color) {
  begin_shader(project_pattern);
  Vector2 sp = {
      (float)cam.sp,
      (float)cam.sp,
  };
  int tmp_size[2] = {tex.width, tex.height};
  int img_size[2] = {szImg.x, szImg.y};
  SetTextureFilter(tex, TEXTURE_FILTER_POINT);
  set_shader_int(project_pattern, pattern, &pattern);
  set_shader_int(project_pattern, pad, &pad);
  set_shader_vec2(project_pattern, sp, &sp);
  set_shader_ivec2(project_pattern, img_size, &tmp_size);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = cam.off.x;
  float cy = cam.off.y;
  float cs = cam.sp;
  DrawTexturePro(
      tex,
      (Rectangle){0, (tmp_size[1] - (0 + img_size[1])), (float)mw, (float)-mh},
      (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0, color);
  end_shader();
}

/*
 * Encodes a multi-layer chip into a single PNG, without having to rely on
 * multi-frames or on png tags.
 *
 * Assumes a logarithm pyramid layer, ie, each image should be half the size
 * of the image at its bottom. Example: 512x512 , 256x256, 128x128.
 *
 *
 * The way it is done is via smartly using the alpha channels: the circuit
 * itself doesnt need the alpha channels, it just needs the RGB, so we store
 * encoded information on alpha (actually just a few bytes from the alpha
 * channel so we don't deform the image).
 *
 * The alpha channel of the first pixel will define the encoding schema:
 * alpha = 255 --> no encoding.
 * alpha = 254 --> encoding V1
 *
 * For v1 encoding we just concatenate the layers vertically.
 * The second pixel will contain the number of images.:
 * pix[1].a = 255 - num_images.
 */
Image image_encode_layers_log(int n, Image* layers) {
  /* Single layers are treated as regular images */
  if (n == 1) {
    Image out = clone_image(layers[0]);
    image_add_blacks(out);
    return out;
  }
  /* With more layers we store like a mip texture tree */
  int ho = layers[0].height;
  int wo = layers[0].width;
  assert(n <= MAX_LAYERS);
  assert(wo % 8 == 0);
  Image out = GenImageColor(wo, ho + ho / 2, BLACK);
  Color* po = out.data;
  for (int l = 0; l < n; l++) {
    Color* pi = layers[l].data;
    int wi = layers[l].width;
    int hi = layers[l].height;
    assert(wi == (wo >> l));
    assert(hi == (ho >> l));
    for (int yi = 0; yi < hi; yi++) {
      int yo, xo;
      if (l == 0) {
        yo = 0;
        xo = 0;
      } else {
        yo = ho;
        xo = 0;
        for (int ll = 1; ll < l; ll++) {
          xo += wo >> ll;
        }
      }
      Color* ppo = &po[(yo + yi) * wo + xo];
      Color* ppi = &pi[yi * wi];
      for (int x = 0; x < wi; x++) {
        ppo[x].r = ppi[x].r;
        ppo[x].g = ppi[x].g;
        ppo[x].b = ppi[x].b;
        /* bg exported as BLACK instead of blank */
        po[x].a = 255;
      }
    }
  }
  /* step 2: encode size and version. */
  int version = 1;
  po[0].a = 255 - LAYER_MAGIC_1;  // Magic 1
  po[1].a = 255 - LAYER_MAGIC_2;  // Magic 2
  po[2].a = 255 - LAYER_MAGIC_3;  // Magic 3
  po[3].a = 255 - version;        // Version
  po[4].a = 255 - n;              // Number of images
  return out;
}

/*
 * Encoded image when layers have equal size.
 * Images are concatenated vertically.
 */
Image image_encode_layers_const(int n, Image* layers) {
  /* Single layers are treated as regular images */
  if (n == 1) {
    Image out = clone_image(layers[0]);
    image_add_blacks(out);
    return out;
  }
  /* With more layers we store like a mip texture tree */
  int ho = layers[0].height;
  int wo = layers[0].width;
  assert(n <= MAX_LAYERS);
  // assert(wo % 8 == 0);
  Image out = GenImageColor(wo, n * ho, BLACK);
  Color* po = out.data;
  for (int l = 0; l < n; l++) {
    Color* pi = layers[l].data;
    int wi = layers[l].width;
    int hi = layers[l].height;
    assert(wi == wo);
    assert(hi == ho);
    for (int yi = 0; yi < hi; yi++) {
      int xo = 0;
      int yo = l * ho + yi;
      Color* ppo = &po[yo * wo + xo];
      Color* ppi = &pi[yi * wi];
      for (int x = 0; x < wi; x++) {
        ppo[x].r = ppi[x].r;
        ppo[x].g = ppi[x].g;
        ppo[x].b = ppi[x].b;
        /* bg exported as BLACK instead of blank */
        po[x].a = 255;
      }
    }
  }
  /* step 2: encode size and version. */
  int version = 1;
  po[0].a = 255 - LAYER_MAGIC_1;  // Magic 1
  po[1].a = 255 - LAYER_MAGIC_2;  // Magic 2
  po[2].a = 255 - LAYER_MAGIC_3;  // Magic 3
  po[3].a = 255 - version;        // Version
  po[4].a = 255 - n;              // Number of images
  return out;
}

/*
 * Ensures the size is a multiple of MOF
 * This is important to ensure that layers can be created.
 */
Image ensure_size_multiple_of(Image img, int mof) {
  int prev_h = img.height;
  int prev_w = img.width;
  int next_w = mof * ((prev_w + mof - 1) / mof);
  int next_h = mof * ((prev_h + mof - 1) / mof);
  if (next_w != img.width || next_h != img.height) {
    Image new_img = gen_image_filled(next_w, next_h, BLANK);
    Color* color_prv = get_pixels(img);
    Color* color_nxt = get_pixels(new_img);
    for (int y = 0; y < prev_h; y++) {
      for (int x = 0; x < prev_w; x++) {
        color_nxt[y * next_w + x] = color_prv[y * prev_w + x];
      }
    }
    UnloadImage(img);
    return new_img;
  }
  return img;
}

/*
 * Decodes an exported image.
 * Should be used in ingress for example.
 */
void image_decode_layers_log(Image img, int* nl, Image* layers) {
  int wi = img.width;
  int hi = img.height;
  Color* po = img.data;
  bool encoded = false;
  if (img.width * img.height > 4) {
    int m1 = 255 - po[0].a;
    int m2 = 255 - po[1].a;
    int m3 = 255 - po[2].a;
    encoded = ((LAYER_MAGIC_1 == m1) && (LAYER_MAGIC_2 == m2) &&
               (LAYER_MAGIC_3 == m3));
  }
  if (!encoded) {
    Image out = clone_image(img);
    image_remove_blacks(&out);
    layers[0] = out;
    *nl = 1;
    return;
  }
  int version = 255 - po[3].a;
  int n = 255 - po[4].a;
  assert(version == 1);
  assert(n > 1);
  assert(n <= 4);
  int wo = img.width;
  assert(img.height % 3 == 0);
  int ho = 2 * img.height / 3;

  for (int l = 0; l < n; l++) {
    int wi = wo >> l;
    int hi = ho >> l;
    Image out = GenImageColor(wi, hi, BLANK);
    Color* pi = out.data;
    for (int yi = 0; yi < hi; yi++) {
      int yo, xo;
      if (l == 0) {
        yo = 0;
        xo = 0;
      } else {
        yo = ho;
        xo = 0;
        for (int ll = 1; ll < l; ll++) {
          xo += wo >> ll;
        }
      }
      Color* ppi = &pi[yi * wi];
      Color* ppo = &po[(yo + yi) * wo + xo];
      for (int x = 0; x < wi; x++) {
        u8 r = ppo[x].r;
        u8 g = ppo[x].g;
        u8 b = ppo[x].b;
        ppi[x].r = r;
        ppi[x].g = g;
        ppi[x].b = b;
        /* Internal images expect BLANK instead of black*/
        ppi[x].a = (r > 0 || g > 0 || b > 0) ? 255 : 0;
      }
    }
    layers[l] = out;
  }
  *nl = n;
}

void image_decode_layers_const(Image img, int* nl, Image* layers) {
  int wi = img.width;
  int hi = img.height;
  Color* po = img.data;
  bool encoded = false;
  if (img.width * img.height > 4) {
    int m1 = 255 - po[0].a;
    int m2 = 255 - po[1].a;
    int m3 = 255 - po[2].a;
    encoded = ((LAYER_MAGIC_1 == m1) && (LAYER_MAGIC_2 == m2) &&
               (LAYER_MAGIC_3 == m3));
  }
  if (!encoded) {
    Image out = clone_image(img);
    image_remove_blacks(&out);
    layers[0] = out;
    *nl = 1;
    return;
  }
  int version = 255 - po[3].a;
  int n = 255 - po[4].a;
  assert(version == 1);
  assert(n > 1);
  assert(n <= MAX_LAYERS);
  int wo = img.width;
  int ho = img.height / n;
  assert(img.height % ho == 0);

  int en = n;
  if (en > MAX_LAYERS) {
    printf("WARNING: Image has too many layers");
    en = n;
  }
  for (int l = 0; l < en; l++) {
    int wi = wo;
    int hi = ho;
    Image out = GenImageColor(wi, hi, BLANK);
    Color* pi = out.data;
    for (int yi = 0; yi < hi; yi++) {
      int xo = 0;
      int yo = l * ho + yi;
      Color* ppi = &pi[yi * wi];
      Color* ppo = &po[yo * wo + xo];
      for (int x = 0; x < wi; x++) {
        u8 r = ppo[x].r;
        u8 g = ppo[x].g;
        u8 b = ppo[x].b;
        ppi[x].r = r;
        ppi[x].g = g;
        ppi[x].b = b;
        /* Internal images expect BLANK instead of black*/
        ppi[x].a = (r > 0 || g > 0 || b > 0) ? 255 : 0;
      }
    }
    layers[l] = out;
  }
  *nl = n;
}

void draw_tex2(Texture2D tex, Color c) {
  Rectangle src = {0, 0, tex.width, -tex.height};
  Rectangle tgt = {0, 0, tex.width, tex.height};
  DrawTexturePro(tex, src, tgt, (Vector2){0, 0}, 0, c);
}

void draw_tex(Texture2D tex) { draw_tex2(tex, WHITE); }

void draw_stretched(Texture src, RenderTexture2D dst, Color c) {
  int sw = src.width;
  int sh = src.height;
  int dw = dst.texture.width;
  int dh = dst.texture.height;
  Rectangle r_src = {0, 0, sw, -sh};
  Rectangle r_tgt = {0, 0, dw, dh};
  DrawTexturePro(src, r_src, r_tgt, (Vector2){0, 0}, 0, c);
}

void gaussian(int dir, Texture2D in, RenderTexture2D out) {
  BeginTextureMode(out);
  ClearBackground(BLANK);
  begin_shader(gaussian);
  int size[] = {in.width, in.height};
  set_shader_ivec2(gaussian, size, &size);
  set_shader_int(gaussian, size, &size);
  set_shader_int(gaussian, dir, &dir);
  SetTextureWrap(in, TEXTURE_WRAP_MIRROR_CLAMP);
  draw_stretched(in, out, WHITE);
  end_shader();
  EndTextureMode();
}

Image invert_image_v(Image img) {
  Image inv_img = ImageCopy(img);
  Color* cin = img.data;
  Color* cout = inv_img.data;
  int h = img.height;
  int w = img.width;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      cout[y * w + x] = cin[(h - y - 1) * w + x];
    }
  }
  return inv_img;
}
#if 0

IntTexture inttex_create(int width, int height) {
  IntTexture out = {
      .id = 0,
      .width = width,
      .height = height,
  };
  glGenTextures(1, &out.id);
  glBindTexture(GL_TEXTURE_2D, out.id);
  // Use GL_R32I for single-channel 32-bit signed integer
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER,
               GL_INT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  return out;
}

void inttex_update(IntTexture* t, int* data) {
  glBindTexture(GL_TEXTURE_2D, t->id);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->width, t->height, GL_RED_INTEGER,
                  GL_INT, data);
  glBindTexture(GL_TEXTURE_2D, 0);
}
void inttex_free(IntTexture* t) { glDeleteTextures(1, &t->id); }
#endif

void image_decode_layers(Image img, int* nl, Image* layers) {
  image_decode_layers_const(img, nl, layers);
}

Image image_encode_layers(int n, Image* layers) {
  return image_encode_layers_const(n, layers);
}

#if 0
// Draws the wire API in the leftr side of the image.
void render_sidepanel(Image* out, Image buffer, Sim* sim, pindef_t* pdef) {
  int w = out->width;
  int y = 0;
  int arrow_w = 6;
  int arrow_h = 5;
  int pin_size = 4;
  Color* pixels = get_pixels(*out);
  int lh = get_font_line_height();
static int arrow_right[] = {
    0, 0, 0, 1, 0, 0,  //
    0, 0, 0, 1, 1, 0,  //
    1, 1, 1, 1, 1, 1,  //
    0, 0, 0, 1, 1, 0,  //
    0, 0, 0, 1, 0, 0,  //
};

static int arrow_left[] = {
    0, 0, 1, 0, 0, 0,  //
    0, 1, 1, 0, 0, 0,  //
    1, 1, 1, 1, 1, 1,  //
    0, 1, 1, 0, 0, 0,  //
    0, 0, 1, 0, 0, 0,  //
};

  fill_image(out, BLANK);
  Color* buffer_pixels = get_pixels(buffer);
  bool has_sim = sim->wl[0] > 0 && !sim->has_errors;
  int* inputs = NULL;
  int* outputs = NULL;
  int ip = 0;
  int op = 0;
  for (int iconn = 0; iconn < pdef->num_conn; iconn++) {
    const char* ctxt = pdef->conn[iconn].name;
    conn_t ctype = pdef->conn[iconn].type;
    int clen = pdef->conn[iconn].len;
    Image img_txt = render_text(ctxt, WHITE);
    for (int i = 0; i < clen; i++) {
      int yy = y + 4 + 2 * i;
      // c is the reference color
      Color ref_color = MAGENTA;
      if (yy < buffer.height) {
        ref_color = buffer_pixels[yy * buffer.width + 0];
      }
      if (COLOR_EQ(ref_color, BLACK) || COLOR_EQ(ref_color, BLANK)) {
        ref_color = WHITE;
      }
      if (has_sim) {
        int wire_state = 0;
        if (ctype == CONN_LVL2IMG) {
          int offdrv = sim->num_nands;
          int wire_id = sim->drv_to_wire[offdrv + op];
          if (wire_id >= 0) {
            wire_state = pulse_unpack_vafter(sim->state.pulses[wire_id]);
          } else {
            wire_state = S_BIT_UNDEFINED;
          }
          op++;
        } else {
          int offskt = 2 * sim->num_nands;
          wire_state = sim->state.skt_values[offskt + ip];
          ip++;
        }
        ref_color = state2color(ref_color, wire_state);
      }
      for (int x = w - pin_size; x < w; x++) {
        float k = ref_color.a / 255.f;
        ref_color =
            (Color){ref_color.r * k, ref_color.g * k, ref_color.b * k, 255};
        pixels[yy * w + x] = ref_color;
      }
    }

    int xs = 0;
    int th = 6 + 2 * clen;
    int ytxt = y + (th - lh) / 2;
    int xtxt = xs + w - pin_size - 4 - img_txt.width - arrow_w;
    RectangleInt r = {0, 0, img_txt.width, img_txt.height};
    copy_image(img_txt, r, out, (Vector2Int){xtxt, ytxt});
    int* arrow = ctype == CONN_LVL2IMG ? arrow_right : arrow_left;
    for (int yy = 0; yy < arrow_h; yy++) {
      for (int xx = 0; xx < arrow_w; xx++) {
        int xr = xx + (xs + w - pin_size - 2 - arrow_w);
        int yr = yy + ytxt + 2;
        int v = arrow[yy * arrow_w + xx];
        if (v == 1) {
          pixels[yr * w + xr] = WHITE;
        }
      }
    }
    UnloadImage(img_txt);
    y += th;
  }
}
#endif

void export_texture(Texture2D tex, const char* fname) {
  Image tmp = LoadImageFromTexture(tex);
  ExportImage(tmp, fname);
  UnloadImage(tmp);
}

Image _gen_thumbnail(int nl, Image* layers, int w, int h) {
  Image out = GenImageColor(w, h, BLANK);
  int ww = layers[0].width;
  int hh = layers[0].height;
  ww = ww > w ? w : ww;
  hh = hh > h ? h : hh;
  for (int i = 0; i < nl; i++) {
    RectangleInt r = {0, 0, ww, hh};
    image_combine(layers[i], r, &out, (Vector2Int){0, 0});
  }
  return out;
}

Image downsample_image(Image img) {
  int w = img.width;
  int h = img.height;
  int ww = w / 2;
  int hh = h / 2;
  Image tmp = GenImageColor(ww, h, BLANK);
  Image out = GenImageColor(ww, hh, BLANK);
  float k[] = {0.25, 0.5, 0.25};
  Color* src = img.data;
  Color* dst = tmp.data;
  Color* dout = out.data;
  /* conv x */
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < ww; x++) {
      int i1 = y * w + 2 * x;
      int i0 = x > 0 ? i1 - 1 : i1;
      int i2 = x < (w - 1) ? i1 + 1 : i1;
      Color c0 = src[i0];
      Color c1 = src[i1];
      Color c2 = src[i2];
      int r = c0.r * k[0] + c1.r * k[1] + c2.r * k[2];
      int g = c0.g * k[0] + c1.g * k[1] + c2.g * k[2];
      int b = c0.b * k[0] + c1.b * k[1] + c2.b * k[2];
      dst[y * ww + x] = (Color){r, g, b, 255};
    }
  }
  /* conv y */
  for (int y = 0; y < hh; y++) {
    for (int x = 0; x < ww; x++) {
      int i1 = 2 * y * ww + x;
      int i0 = y > 0 ? i1 - ww : i1;
      int i2 = y < (h - 1) ? i1 + ww : i1;
      Color c0 = dst[i0];
      Color c1 = dst[i1];
      Color c2 = dst[i2];
      int r = c0.r * k[0] + c1.r * k[1] + c2.r * k[2];
      int g = c0.g * k[0] + c1.g * k[1] + c2.g * k[2];
      int b = c0.b * k[0] + c1.b * k[1] + c2.b * k[2];
      dout[y * ww + x] = (Color){r, g, b, 255};
    }
  }
  UnloadImage(tmp);
  return out;
}

Image downsample_image_old(Image img) {
  int w = img.width;
  int h = img.height;
  int ww = w / 2;
  int hh = h / 2;

  Image out = GenImageColor(ww, hh, BLANK);
  Color* src = img.data;
  Color* dst = out.data;
  for (int y = 0; y < hh; y++) {
    for (int x = 0; x < ww; x++) {
      int idx = (2 * y) * w + 2 * x;
      Color c1 = src[idx + 0];
      Color c2 = src[idx + 1];
      Color c3 = src[idx + w];
      Color c4 = src[idx + w + 1];
      Color c;
      c.r = c1.r / 4 + c2.r / 4 + c3.r / 4 + c4.r / 4;
      c.g = c1.g / 4 + c2.g / 4 + c3.g / 4 + c4.g / 4;
      c.b = c1.b / 4 + c2.b / 4 + c3.b / 4 + c4.b / 4;
      c.a = 255;
      dst[y * ww + x] = c;
    }
  }
  return out;
}

static inline Color linear_sample(Color* data, int w, int h, float fx,
                                  float fy) {
  int x0 = floor(fx);
  int y0 = floor(fy);
  int x1 = x0 + 1;
  int y1 = y0 + 1;
  float wx0 = x1 - fx;
  float wx1 = 1 - wx0;
  float wy0 = y1 - fy;
  float wy1 = 1 - wy0;
  if (x0 < 0) x0 = 0;
  if (x0 >= w) x0 = w - 1;
  if (x1 < 0) x1 = 0;
  if (x1 >= w) x1 = w - 1;
  if (y1 < 0) y1 = 0;
  if (y1 >= h) y1 = h - 1;
  if (y0 < 0) y0 = 0;
  if (y0 >= h) y0 = h - 1;

  Color c00 = data[y0 * w + x0];
  Color c01 = data[y0 * w + x1];
  Color c10 = data[y1 * w + x0];
  Color c11 = data[y1 * w + x1];

  float rr =
      (c00.r * wx0 + c01.r * wx1) * wy0 + (c10.r * wx0 + c11.r * wx1) * wy1;
  float gg =
      (c00.g * wx0 + c01.g * wx1) * wy0 + (c10.g * wx0 + c11.g * wx1) * wy1;
  float bb =
      (c00.b * wx0 + c01.b * wx1) * wy0 + (c10.b * wx0 + c11.b * wx1) * wy1;

  int r = rr;
  int g = gg;
  int b = bb;
  return (Color){r, g, b, 255};
}

static inline Color nearest_sample(Color* data, int w, int h, float fx,
                                   float fy) {
  int x0 = round(fx);
  int y0 = round(fy);
  if (x0 < 0) x0 = 0;
  if (x0 >= w) x0 = w - 1;
  if (y0 < 0) y0 = 0;
  if (y0 >= h) y0 = h - 1;
  return data[y0 * w + x0];
}

Image img_resize(Image img, int ww, int hh, bool linear) {
  Image out = GenImageColor(ww, hh, BLANK);
  int w = img.width;
  int h = img.height;
  Color* src = img.data;
  Color* dst = out.data;
  float fw = w;
  float fh = h;
  float fww = ww;
  float fhh = hh;
  for (int y = 0; y < hh; y++) {
    for (int x = 0; x < ww; x++) {
      float px = ((x + 0.5) / fww) * fw - 0.5;
      float py = ((y + 0.5) / fhh) * fh - 0.5;
      Color c;
      if (linear) {
        c = linear_sample(src, w, h, px, py);
      } else {
        c = nearest_sample(src, w, h, px, py);
      }
      dst[y * ww + x] = c;
    }
  }
  return out;
}

Image img_resize_box(Image img, int ww, int hh) {
  Image out = GenImageColor(ww, hh, BLANK);
  int w = img.width;
  int h = img.height;
  Color* src = img.data;
  Color* dst = out.data;
  float fw = w;
  float fh = h;
  float fww = ww;
  float fhh = hh;
  for (int y = 0; y < hh; y++) {
    for (int x = 0; x < ww; x++) {
      int x0 = (x * w) / ww;
      int x1 = ((x + 1) * w) / ww;
      int y0 = (y * h) / hh;
      int y1 = ((y + 1) * h) / hh;
      int r = 0;
      int g = 0;
      int b = 0;
      int n = 0;
      if (x0 == x1) x1 = x0 + 1;
      if (y0 == y1) y1 = y0 + 1;
      for (int yy = y0; yy < y1; yy++) {
        for (int xx = x0; xx < x1; xx++) {
          Color c = src[(yy)*w + (xx)];
          r += c.r;
          g += c.g;
          b += c.b;
          n++;
        }
      }
      r = r / n;
      g = g / n;
      b = b / n;
      dst[y * ww + x] = (Color){r, g, b, 255};
    }
  }
  return out;
}

Image gen_thumbnail(int nl, Image* layers, int wmax, int hmax) {
  int ww = layers[0].width;
  int hh = layers[0].height;
  if (ww == 0) {
    for (int i = 0; i < nl; i++) {
      int ww2 = layers[i].width;
      if (ww2 > 0) {
        ww = layers[i].width;
        hh = layers[i].height;
        nl = 1;
        layers = &layers[i];
        break;
      }
    }
  }
  Image out = GenImageColor(ww, hh, BLANK);
  for (int i = 0; i < nl; i++) {
    RectangleInt r = {0, 0, ww, hh};
    image_combine(layers[i], r, &out, (Vector2Int){0, 0});
  }
  image_add_blacks(out);

#if 0
  while (true) {
    bool needs_downsample = false;
    if (ww > 2 * wmax) needs_downsample = true;
    if (hh > 2 * hmax) needs_downsample = true;
    if (!needs_downsample) break;
    Image downsampled = downsample_image(out);
    UnloadImage(out);
    ww = out.width;
    hh = out.height;
    out = downsampled;
  }
#endif
  int wo, ho;
  if (ww < wmax && hh < hmax) {
    int f1 = wmax / ww;
    int f2 = hmax / hh;
    f1 = f1 < f2 ? f1 : f2;
    if (f1 == 1) {
      return out;
    } else {
      wo = f1 * ww;
      ho = f1 * hh;
    }
  } else {
    if (ww > hh) {
      wo = wmax;
      ho = (((float)hh) / ww) * wo;
    } else {
      ho = hmax;
      wo = (((float)ww) / hh) * ho;
    }
  }
  // Image resized = img_resize(out, wo, ho, true);
  Image resized = img_resize_box(out, wo, ho);
  UnloadImage(out);
  return resized;
}

void project_with_dist(Cam2D cam, Texture2D img, int dist_type) {
  shader_load("project_with_dist");
  int w = img.width;
  int h = img.height;
  int size[] = {w, h};
  float cx = cam.off.x;
  float cy = cam.off.y;
  float cs = cam.sp;
  Vector2 sp = {cs, cs};
  Vector2 off = {cx, cy};
  shader_vec2("sp", &sp);
  shader_vec2("off", &off);
  shader_ivec2("img_size", &size[0]);
  shader_intv("distance_type", &dist_type, 1);
  DrawTexturePro(img,                                    /* image to draw */
                 (Rectangle){0, 0, (float)w, (float)-h}, /* source */
                 (Rectangle){cx, cy, w * cs, h * cs},    /* target */
                 (Vector2){0, 0}, 0,                     /* origin/ rot*/
                 WHITE);

  end_shader();
}

void project_regular(Cam2D cam, Texture2D img) {
  int w = img.width;
  int h = img.height;
  int size[] = {w, h};
  float cx = cam.off.x;
  float cy = cam.off.y;
  float cs = cam.sp;
  DrawTexturePro(img,                                    /* image to draw */
                 (Rectangle){0, 0, (float)w, (float)-h}, /* source */
                 (Rectangle){cx, cy, w * cs, h * cs},    /* target */
                 (Vector2){0, 0}, 0,                     /* origin/ rot*/
                 WHITE);
}

void naive_bokeh(Texture src) {
  shader_load("naive_bokeh");
  int w = src.width;
  int h = src.height;
  Vector2 tsize = {w, h};
  // shader_vec2("sp", &sp);
  shader_vec2("tsize", &tsize);
  draw_tex(src);
  end_shader();
}

void save_img_u8(int w, int h, u8* data, const char* fname) {
  Image img = GenImageColor(w, h, BLANK);
  Color* clr = img.data;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      int value = data[idx];
      if (value == 0) {
        clr[idx] = BLACK;
      } else if (value == 1) {
        clr[idx] = WHITE;
      } else {
        clr[idx] = RED;
      }
    }
  }
  ExportImage(img, fname);
  UnloadImage(img);
}

void save_img_f32(int w, int h, float* data, float vmin, float vmax,
                  const char* fname) {
  Image img = GenImageColor(w, h, BLANK);
  Color* clr = img.data;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      float v = (data[idx] - vmin) / (vmax - vmin);
      v = 255 * v;
      if (v > 255) v = 255;
      if (v < 0) v = 0;
      u8 c = v;
      clr[idx].r = c;
      clr[idx].g = c;
      clr[idx].b = c;
      clr[idx].a = 255;
    }
  }
  ExportImage(img, fname);
  UnloadImage(img);
}
