#include "pyramid.h"

#include "stdlib.h"

// Left 2 implementations for pyramid: averaging color or "max" color.
// The average one will lead to issues whenever we stick with a limited palette.
// For now, we still use the average.
static Image PyramidGenImageAvg(Image base);
static Image PyramidGenImageMax(Image base);
static void PyramidUpdatePixelAvg(Image b, Image* s, int x2, int y2);
static void PyramidUpdatePixelMax(Image b, Image* s, int x2, int y2);
static void PyramidUpdateRectAvg(Image b, Image* s, RectangleInt r);
static void PyramidUpdateRectMax(Image b, Image* s, RectangleInt r);
static void PyramidUpdateRect2Avg(int n, Image* h, RectangleInt r);
static void PyramidUpdateRect2Max(int n, Image* h, RectangleInt r);

Image PyramidGenImage(Image base)
{
  return PyramidGenImageAvg(base);
}

void PyramidUpdatePixel(Image b, Image* s, int x2, int y2)
{
  PyramidUpdatePixelAvg(b, s, x2, y2);
}

void PyramidUpdateRect(Image b, Image* s, RectangleInt r)
{
  PyramidUpdateRectAvg(b, s, r);
}

void PyramidUpdateRect2(int n, Image* h, RectangleInt r)
{
  PyramidUpdateRect2Avg(n, h, r);
}

Image PyramidGenImageAvg(Image base)
{
  Image sub = GenImageSimple((base.width + 1) / 2, (base.height + 1) / 2);
  PyramidUpdateRectAvg(base, &sub, (RectangleInt){0, 0, sub.width, sub.height});
  return sub;
}

Image PyramidGenImageMax(Image base)
{
  Image sub = GenImageSimple((base.width + 1) / 2, (base.height + 1) / 2);
  PyramidUpdateRectMax(base, &sub, (RectangleInt){0, 0, sub.width, sub.height});
  return sub;
}

void PyramidUpdatePixelAvg(Image b, Image* s, int x2, int y2)
{
  int w1 = b.width;
  int h1 = b.height;
  int w2 = s->width;
  Color* d1 = GetPixels(b);
  Color* d2 = GetPixels(*s);
  int x1 = x2 << 1;
  int y1 = y2 << 1;
  int idx = y1 * w1 + x1;
  int vr = 0;
  int vg = 0;
  int vb = 0;
  int va = 0;

  vr += d1[idx + 0].r;
  vg += d1[idx + 0].g;
  vb += d1[idx + 0].b;
  va += d1[idx + 0].a;
  if (x1 + 1 < w1) {
    vr += d1[idx + 1].r;
    vg += d1[idx + 1].g;
    vb += d1[idx + 1].b;
    va += d1[idx + 1].a;
  }
  if (y1 + 1 < h1) {
    vr += d1[idx + w1 + 0].r;
    vg += d1[idx + w1 + 0].g;
    vb += d1[idx + w1 + 0].b;
    va += d1[idx + w1 + 0].a;
    if (x1 + 1 < w1) {
      vr += d1[idx + w1 + 1].r;
      vg += d1[idx + w1 + 1].g;
      vb += d1[idx + w1 + 1].b;
      va += d1[idx + w1 + 1].a;
    }
  }
  d2[y2 * w2 + x2] = (Color){vr >> 2, vg >> 2, vb >> 2, va >> 2};
}

void PyramidUpdatePixelMax(Image b, Image* s, int x2, int y2)
{
  int w1 = b.width;
  int h1 = b.height;
  int w2 = s->width;
  Color* d1 = GetPixels(b);
  Color* d2 = GetPixels(*s);
  int x1 = x2 << 1;
  int y1 = y2 << 1;
  int idx = y1 * w1 + x1;
  Color c = d1[idx];
  if (x1 + 1 < w1 && d1[idx + 1].r > c.r) {
    c = d1[idx + 1];
  }
  if (y1 + 1 < h1 && d1[idx + w1 + 0].r > c.r) {
    c = d1[idx + w1 + 0];
    if (x1 + 1 < w1 && d1[idx + w1 + 1].r > c.r) {
      c = d1[idx + w1 + 1];
    }
  }
  d2[y2 * w2 + x2] = c;
}

void PyramidUpdateRectAvg(Image b, Image* s, RectangleInt r)
{
  int ux2 = r.x;
  int uy2 = r.y;
  int uw2 = r.width;
  int uh2 = r.height;
  if (ux2 + uw2 > s->width) abort();
  if (uy2 + uh2 > s->height) abort();
  int w1 = b.width;
  int h1 = b.height;
  int w2 = s->width;
  Color* d1 = GetPixels(b);
  Color* d2 = GetPixels(*s);
  for (int y2 = uy2; y2 < uh2 + uy2; y2++) {
    for (int x2 = ux2; x2 < uw2 + ux2; x2++) {
      int x1 = x2 << 1;
      int y1 = y2 << 1;
      int idx = y1 * w1 + x1;
      int vr = 0;
      int vg = 0;
      int vb = 0;
      int va = 0;
      vr += d1[idx + 0].r;
      vg += d1[idx + 0].g;
      vb += d1[idx + 0].b;
      va += d1[idx + 0].a;
      if (x1 + 1 < w1) {
        vr += d1[idx + 1].r;
        vg += d1[idx + 1].g;
        vb += d1[idx + 1].b;
        va += d1[idx + 1].a;
      }
      if (y1 + 1 < h1) {
        vr += d1[idx + w1 + 0].r;
        vg += d1[idx + w1 + 0].g;
        vb += d1[idx + w1 + 0].b;
        va += d1[idx + w1 + 0].a;
        if (x1 + 1 < w1) {
          vr += d1[idx + w1 + 1].r;
          vg += d1[idx + w1 + 1].g;
          vb += d1[idx + w1 + 1].b;
          va += d1[idx + w1 + 1].a;
        }
      }
      d2[y2 * w2 + x2] = (Color){vr >> 2, vg >> 2, vb >> 2, va >> 2};
    }
  }
}

void PyramidUpdateRectMax(Image b, Image* s, RectangleInt r)
{
  int ux2 = r.x;
  int uy2 = r.y;
  int uw2 = r.width;
  int uh2 = r.height;
  if (ux2 + uw2 > s->width) abort();
  if (uy2 + uh2 > s->height) abort();
  int w1 = b.width;
  int h1 = b.height;
  int w2 = s->width;
  Color* d1 = GetPixels(b);
  Color* d2 = GetPixels(*s);
  for (int y2 = uy2; y2 < uh2 + uy2; y2++) {
    for (int x2 = ux2; x2 < uw2 + ux2; x2++) {
      int x1 = x2 << 1;
      int y1 = y2 << 1;
      int idx = y1 * w1 + x1;
      Color c = d1[idx];
      if (x1 + 1 < w1 && ColorToGray(d1[idx + 1]) > ColorToGray(c)) {
        c = d1[idx + 1];
      }
      if (y1 + 1 < h1 && ColorToGray(d1[idx + w1 + 0]) > ColorToGray(c)) {
        c = d1[idx + w1 + 0];
        if (x1 + 1 < w1 && ColorToGray(d1[idx + w1 + 1]) > ColorToGray(c)) {
          c = d1[idx + w1 + 1];
        }
      }
      d2[y2 * w2 + x2] = c;
    }
  }
}

void PyramidUpdateRect2Avg(int n, Image* h, RectangleInt r)
{
  int x0 = r.x;
  int y0 = r.y;
  int x1 = r.width + x0;
  int y1 = r.height + y0;
  for (int i = 1; i < n; i++) {
    x0 = x0 >> 1;
    y0 = y0 >> 1;
    x1 = x1 >> 1;
    y1 = y1 >> 1;
    PyramidUpdateRectAvg(h[i - 1], &h[i], (RectangleInt){x0, y0, x1 - x0, y1 - y0});
  }
}

void PyramidUpdateRect2Max(int n, Image* h, RectangleInt r)
{
  int x0 = r.x;
  int y0 = r.y;
  int x1 = r.width + x0;
  int y1 = r.height + y0;
  for (int i = 1; i < n; i++) {
    x0 = x0 >> 1;
    y0 = y0 >> 1;
    x1 = x1 >> 1;
    y1 = y1 >> 1;
    PyramidUpdateRectMax(h[i - 1], &h[i], (RectangleInt){x0, y0, x1 - x0, y1 - y0});
  }
}
