#include "img.h"

#include <stdio.h>

#include "assert.h"
#include "colors.h"
#include "font.h"
#include "stdlib.h"

// Makes sure the alpha is 255
static Color ToValidColor(Color c)
{
  return (Color){c.r, c.g, c.b, 255};
}

static bool IsRectInside(Image img, RectangleInt region)
{
  RectangleInt intersection = GetCollisionRecInt(GetImageRect(img), region);
  bool ok = true;
  ok = ok && (intersection.x == region.x);
  ok = ok && (intersection.y == region.y);
  ok = ok && (intersection.height == region.height);
  ok = ok && (intersection.width == region.width);
  return ok;
}

Color* GetPixels(Image img)
{
  if (img.width == 0 || img.height == 0) {
    return NULL;
  }
  assert(img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  return (Color*)img.data;
}

// It's just to make it easier to compare colors
int* GetPixelsAsInt(Image img)
{
  assert(img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  return (int*)img.data;
}

RectangleInt GetImageRect(Image img)
{
  return (RectangleInt){0, 0, img.width, img.height};
}

Image GenImageSimple(int w, int h)
{
  // Keeping this function because eventually we might want to enforce the default image data type (like keeping a palette).
  return GenImageFilled(w, h, BLACK);
}

Image GenImageFilled(int w, int h, Color v)
{
  return GenImageColor(w, h, v);
}

Image CloneImage(Image img)
{
  return CropImage(img, GetImageRect(img));
}

void FillImage(Image* img, Color v)
{
  int size = img->width * img->height;
  Color* color_data = GetPixels(*img);
  for (int i = 0; i < size; i++) {
    color_data[i] = v;
  }
}

Image CropImage(Image img, RectangleInt region)
{
  assert(IsRectInside(img, region));  // Checks that the cropping region is inside the image
  assert(!IsRecIntEmpty(region));     // cropped region shouldn't be empty
  int w = region.width;
  int h = region.height;
  Image cropped = GenImageSimple(w, h);
  Color* cropped_pixels = GetPixels(cropped);
  Color* img_pixels = GetPixels(img);
  for (int y = 0; y < h; y++) {
    int off_img = (y + region.y) * img.width + (region.x);
    int off_cropped = y * cropped.width;
    for (int x = 0; x < w; x++) {
      cropped_pixels[off_cropped + x] = img_pixels[off_img + x];
    }
  }
  return cropped;
}

Image RotateImage(Image img, int ccw)
{
  Image out = GenImageSimple(img.height, img.width);
  Color* pout = GetPixels(out);
  Color* pin = GetPixels(img);
  for (int y = 0; y < img.height; y++) {
    for (int x = 0; x < img.width; x++) {
      if (ccw == 1) {
        int pix_out = (img.width - x - 1) * out.width + y;
        int pix_in = y * img.width + x;
        pout[pix_out] = pin[pix_in];
      }
      else {
        int pix_in = y * img.width + x;
        int pix_out = x * out.width + (img.height - y - 1);
        pout[pix_out] = pin[pix_in];
      }
    }
  }
  return out;
}

void FlipImageHInplace(Image* img)
{
  Color* colors = GetPixels(*img);
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

void FlipImageVInplace(Image* img)
{
  Color* colors = GetPixels(*img);
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

void FillImageRect(Image* img, RectangleInt r, Color c)
{
  assert(IsRectInside(*img, r));
  int x0 = r.x;
  int y0 = r.y;
  int w = r.width;
  int h = r.height;
  int ww = img->width;
  Color* colors = GetPixels(*img);
  for (int y = y0; y < y0 + h; y++) {
    for (int x = x0; x < x0 + w; x++) {
      int p = y * ww + x;
      colors[p] = c;
    }
  }
}

void ImageCombine(Image src, RectangleInt r, Image* dst, Vector2Int offset)
{
  assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= src.width && r.y + r.height <= src.height);
  assert(offset.x >= 0 && offset.y >= 0 && offset.x + r.width <= dst->width && offset.y + r.height <= dst->height);
  Color* csrc = GetPixels(src);
  Color* cdst = GetPixels(*dst);
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
      }
      else if (!COLOR_EQ(csrc[p1], BLANK)) {
        cdst[p2] = csrc[p1];
      }
    }
  }
}

void CopyImage(Image src, RectangleInt r, Image* dst, Vector2Int offset)
{
  assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= src.width && r.y + r.height <= src.height);
  assert(offset.x >= 0 && offset.y >= 0 && offset.x + r.width <= dst->width && offset.y + r.height <= dst->height);
  Color* csrc = GetPixels(src);
  Color* cdst = GetPixels(*dst);
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
      }
      else {
        cdst[p2] = csrc[p1];
      }
    }
  }
}

void ImageRemoveBlacks(Image* img)
{
  Color* colors = GetPixels(*img);
  for (int y = 0; y < img->height; y++) {
    for (int x = 0; x < img->width; x++) {
      int p = y * img->width + x;
      int c = ColorToGray(colors[p]);
      if (c < 1) {
        colors[p] = BLANK;
      }
      else {
        colors[p] = ToValidColor(colors[p]);
      }
    }
  }
}

void ImageAddBlacks(Image img)
{
  Color* colors = GetPixels(img);
  for (int y = 0; y < img.height; y++) {
    for (int x = 0; x < img.width; x++) {
      int p = y * img.width + x;
      int c = ColorToGray(colors[p]);
      if (c > 1) {
        colors[p] = ToValidColor(colors[p]);
      }
      else {
        colors[p] = BLACK;
      }
    }
  }
}

void ImageEnsureMaxSize(Image* img)
{
  int max_size = MAX_IMG_SIZE;
  if (img->width > max_size || img->height > max_size) {
    int w = img->width;
    int h = img->height;
    int w2 = (w > max_size) ? max_size : w;
    int h2 = (h > max_size) ? max_size : h;
    Image smaller_img = GenImageSimple(w2, h2);
    Color* p1 = GetPixels(*img);
    Color* p2 = GetPixels(smaller_img);
    for (int y = 0; y < h2; y++) {
      for (int x = 0; x < w2; x++) {
        p2[y * w2 + x] = p1[y * w + x];
      }
    }
    UnloadImage(*img);
    *img = smaller_img;
  }
}

void DrawImageLineTool(Vector2Int start, RectangleInt tool_rect, RectangleInt img_rect, int ls, bool corner, bool end_corner, Color c, Image* out, Vector2Int* off)
{
  const int DIR_HORIZONTAL = 0;
  const int DIR_VERTICAL = 1;
  RectangleInt r = tool_rect;
  int startx = start.x;
  int starty = start.y;
  int endx = 2 * r.x + r.width - startx - 1;
  int endy = 2 * r.y + r.height - starty - 1;

  int dir = -1;
  if (r.width >= r.height) {
    dir = DIR_HORIZONTAL;
    r = (RectangleInt){
        .x = r.x,
        .y = starty,
        .width = r.width,
        .height = 2 * ls - 1,
    };
  }
  else {
    dir = DIR_VERTICAL;
    r = (RectangleInt){
        .x = startx,
        .y = r.y,
        .width = 2 * ls - 1,
        .height = r.height,
    };
  }
  r = GetCollisionRecInt(r, img_rect);
  if (r.width == 0 || r.height == 0) {
    *out = (Image){0};
    *off = (Vector2Int){0};
    return;
  }
  int w = r.width;
  int h = r.height;
  Image img = GenImageFilled(w, h, BLANK);
  *out = img;
  // horizontal
  Color* data = GetPixels(*out);
  if (dir == DIR_HORIZONTAL) {
    for (int y = 0; y < img.height; y += 2) {
      Color* line = data + y * img.width;
      for (int x = 0; x < img.width; x++) {
        // Horizontal lines
        // When end_corner is true, it's like it starts from the other end.
        if (corner) {
          int dx = x + r.x - startx;
          int dy = y + r.y - starty;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (endx > startx && dx < dy) continue;
          if (endx < startx && dx < (h - dy - 1)) continue;
        }
        if (end_corner) {
          // Just exchange startx and endx
          int dx = x + r.x - endx;
          int dy = y + r.y - starty;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (startx > endx && dx < dy) continue;
          if (startx < endx && dx < (h - dy - 1)) continue;
        }
        line[x] = c;
      }
    }
  }
  else {
    for (int y = 0; y < img.height; y++) {
      Color* line = data + y * img.width;
      for (int x = 0; x < img.width; x += 2) {
        if (corner) {
          int dx = x + r.x - startx;
          int dy = y + r.y - starty;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (endy > starty && dx > dy) continue;
          if (endy < starty && (w - dx - 1) > dy) continue;
        }
        if (end_corner) {
          int dx = x + r.x - startx;
          int dy = y + r.y - endy;
          dx = dx < 0 ? -dx : dx;
          dy = dy < 0 ? -dy : dy;
          if (starty > endy && dx > dy) continue;
          if (starty < endy && (w - dx - 1) > dy) continue;
        }
        line[x] = c;
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
      r.y -= img.height - 1;
    }
  }
  if (dir == DIR_VERTICAL) {
    if (endx < startx) {
      r.x -= img.width - 1;
    }
  }

  off->x = r.x;
  off->y = r.y;
}

static inline int GetCrossingPixelDirection(int* pixels, int w, int h, int x, int y)
{
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
  }
  else {
    return 0;
  }
}

static inline bool IsCrossing(int* pixels, int w, int h, int x, int y)
{
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

void DrawImageBucketTool(Image img, int x, int y, int sw, int sh, Color c, Image* out, Vector2Int* off)
{
  // Idea: Like a floodfill but only wire logic.
  // A bit tricky. Need to identify how it would be displayed in the
  // simulation.

  // Each pixel is either:
  // - a crossing
  // - a line
  // - a T
  // Maybe I can do as in the simu parsing, at each time compute only the
  // vertical or horizontal components
  //
  // Line sections = lines that extend until finding black. (vertical or horizontal)
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
  int* pixels = GetPixelsAsInt(img);
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
      int d = GetCrossingPixelDirection(pixels, w, h, xx, yy);
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
     *           if not IsCrossing(x, y) then
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
        if (!IsCrossing(pixels, w, h, xx, yy)) {
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
        if (!IsCrossing(pixels, w, h, xx, yy)) {
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
  Image rimg = GenImageFilled(ww, hh, BLANK);
  *out = rimg;
  Color* out_pixels = GetPixels(rimg);
  for (int iy = 0; iy < hh; iy++) {
    for (int ix = 0; ix < ww; ix++) {
      int idx = (iy + ymin) * w + (ix + xmin);
      int ih = draft[2 * idx + 0];
      int iv = draft[2 * idx + 1];
      int n = ih + iv;
      if (n == 0) continue;
      int oidx = iy * ww + ix;
      if (n == 2) {
        out_pixels[oidx] = c;
      }
      else {
        if (GetCrossingPixelDirection(pixels, w, h, ix + xmin, iy + ymin)) {
          if (draft[2 * idx + 1]) out_pixels[oidx] = c;
        }
        else {
          if (draft[2 * idx + 0]) out_pixels[oidx] = c;
        }
      }
    }
  }
  free(Q);
  free(draft);
}

void DrawImageRectSimple(Image* img, int x, int y, int w, int h, Color c)
{
  assert(IsRectInside(*img, (RectangleInt){x, y, w, h}));
  int x0 = x;
  int y0 = y;
  int ww = img->width;
  Color* colors = GetPixels(*img);
  for (int y = y0; y < y0 + h; y++) {
    colors[y * ww + x0] = c;
    colors[y * ww + x0 + w - 1] = c;
  }
  for (int x = x0; x < x0 + w; x++) {
    colors[y0 * ww + x] = c;
    colors[(y0 + h - 1) * ww + x] = c;
  }
}
