#include "rendering.h"

#include <math.h>

#include "colors.h"
#include "font.h"
#if defined(WITH_OPENMP)
#include <omp.h>
#endif
#include <string.h>

void RenderImageSimple(Image* out, Image img, float pixel_size, int camera_x,
                       int camera_y, int offx, int offy) {
  int w = out->width;
  int h = out->height;
  float pinv = 1.f / pixel_size;
  Color* pin = GetPixels(img);
  Color* pout = GetPixels(*out);

  int xmin = (0 + offx) * pixel_size - camera_x - pixel_size;
  int ymin = (0 + offy) * pixel_size - camera_y - pixel_size;
  int xmax = (img.width + offx) * pixel_size - camera_x + pixel_size;
  int ymax = (img.height + offy) * pixel_size - camera_y + pixel_size;

  int x0 = MaxInt(MinInt(xmin, w), 0);
  int x1 = MaxInt(MinInt(xmax, w), 0);
  int y0 = MaxInt(MinInt(ymin, h), 0);
  int y1 = MaxInt(MinInt(ymax, h), 0);

  int y;
#pragma omp parallel for
  for (y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      int px = floor((x + camera_x) * pinv) - offx;
      int py = floor((y + camera_y) * pinv) - offy;
      if (px >= 0 && px <= img.width && py >= 0 && py < img.height) {
        Color c = pin[py * img.width + px];
        if (c.a > 0) {
          pout[y * w + x] = c;
        }
      }
    }
  }
}

void RenderImageEdit(RenderImgCtx r) {
  int w = r.out.width;
  int h = r.out.height;
  float pinv = 1.f / r.pixel_size;
  Image img = r.img[0];
  bool has_sel = r.sel[0].width > 0;
  Image sel = has_sel ? r.sel[0] : (Image){0};
  bool has_tool = r.tool_img.width > 0;

  Color* pin = GetPixels(img);
  Color* ptool = GetPixels(r.tool_img);
  Color* pins = GetPixels(sel);
  Color* pout = GetPixels(r.out);
  Image img2 = r.img[1];
  Image sel2 = has_sel ? r.sel[1] : (Image){0};
  Color* pin2 = GetPixels(r.img[1]);
  Color* pins2 = GetPixels(sel2);

  Image img4 = r.img[2];
  Image sel4 = has_sel ? r.sel[2] : (Image){0};
  Color* pin4 = GetPixels(img4);
  Color* pins4 = GetPixels(sel4);

  int xmin = (0) * r.pixel_size - r.camera_x - r.pixel_size;
  int ymin = (0) * r.pixel_size - r.camera_y - r.pixel_size;
  int xmax = (img.width) * r.pixel_size - r.camera_x + r.pixel_size;
  int ymax = (img.height) * r.pixel_size - r.camera_y + r.pixel_size;

  int y;

  int x0 = MaxInt(MinInt(xmin, w), 0);
  int x1 = MaxInt(MinInt(xmax, w), 0);
  int y0 = MaxInt(MinInt(ymin, h), 0);
  int y1 = MaxInt(MinInt(ymax, h), 0);

#pragma omp parallel for
  for (y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      int px = floor((x + r.camera_x) * pinv);
      int py = floor((y + r.camera_y) * pinv);
      if (px >= 0 && px < img.width && py >= 0 && py < img.height) {
        if (pinv <= 1.001) {
          pout[y * w + x] = pin[py * img.width + px];
          if (r.grid) {
            int pxp = floor((x - 1 + r.camera_x) * pinv);
            int pyp = floor((y - 1 + r.camera_y) * pinv);
            if (px != pxp || py != pyp) {
              pout[y * w + x] = r.grid_color;
            }
          }
        } else if (pinv < 2.001) {
          int py2 = py >> 1;
          int px2 = px >> 1;
          pout[y * w + x] = pin2[py2 * img2.width + px2];
        } else {
          int py4 = py >> 2;
          int px4 = px >> 2;
          pout[y * w + x] = pin4[py4 * img4.width + px4];
        }
        // Selection buffer
        if (has_sel) {
          int pxs = px - r.sel_off_x;
          int pys = py - r.sel_off_y;
          if (pxs >= 0 && pxs < r.sel[0].width && pys >= 0 &&
              pys < r.sel[0].height) {
            Color coff;
            if (pinv <= 1.001) {
              coff = pins[pys * r.sel[0].width + pxs];
            } else if (pinv < 2.001) {
              int pxs2 = pxs >> 1;
              int pys2 = pys >> 1;
              coff = pins2[pys2 * sel2.width + pxs2];
            } else {
              int pxs4 = pxs >> 2;
              int pys4 = pys >> 2;
              coff = pins4[pys4 * sel4.width + pxs4];
            }

            if (coff.a > 0) {
              pout[y * w + x] = coff;
            }
          }
        }
        // tool preview
        if (has_tool) {
          int pxs = px - r.tool_off_x;
          int pys = py - r.tool_off_y;
          if (pxs >= 0 && pxs < r.tool_img.width && pys >= 0 &&
              pys < r.tool_img.height) {
            Color coff = ptool[pys * r.tool_img.width + pxs];
            if (coff.a > 0) {
              pout[y * w + x] = coff;
            }
          }
        }
        if (r.pixel_preview) {
          if (px == r.pixel_preview_x && py == r.pixel_preview_y) {
            pout[y * w + x] = r.pixel_preview_color;
          }
        }
      } else {
        // pout[y * w + x] = r.bg;
      }
    }
  }
}

void RenderImageSelRect(Image* out, float pixel_size, int camera_x,
                        int camera_y, RectangleInt r, int ls, double t) {
  int w = out->width;
  int h = out->height;

  // pixel top-corner for a given pixel position
  int x0 = r.x * pixel_size - camera_x - ls;
  int y0 = r.y * pixel_size - camera_y - ls;
  int x1 = (r.x + r.width) * pixel_size - camera_x + ls - 1;
  int y1 = (r.y + r.height) * pixel_size - camera_y + ls - 1;

  int y0b = MinInt(MaxInt(y0, 0), h - 1);
  int y1b = MinInt(MaxInt(y1, 0), h - 1);
  int x0b = MinInt(MaxInt(x0, 0), w - 1);
  int x1b = MinInt(MaxInt(x1, 0), w - 1);
  Color lut[] = {
      WHITE,
      BLACK,
  };

  int k = 20 * t;
  k = k & 15;
  k = 16 - k;
  Color* pixels = GetPixels(*out);
  // Upper line
  if (y0 == y0b) {
    for (int x = x0b; x <= x1b; x++) {
      pixels[y0 * w + x] = lut[((k + x + y0) >> 3) % 2];
    }
  }
  if (y1 == y1b) {
    for (int x = x0b; x <= x1b; x++) {
      pixels[y1 * w + x] = lut[((k + x + y1) >> 3) % 2];
    }
  }
  if (x0 == x0b) {
    for (int y = y0b; y <= y1b; y++) {
      pixels[y * w + x0] = lut[((k + x0 + y) >> 3) % 2];
    }
  }
  if (x1 == x1b) {
    for (int y = y0b; y <= y1b; y++) {
      pixels[y * w + x1] = lut[((k + x1 + y) >> 3) % 2];
    }
  }
}

void DrawImageSceneRect(Image* out, float pixel_size, int camera_x,
                        int camera_y, Rectangle r, Color c) {
  int w = out->width;
  int h = out->height;

  // pixel top-corner for a given pixel position
  int x0 = r.x * pixel_size - camera_x;
  int y0 = r.y * pixel_size - camera_y;
  int x1 = (r.x + r.width) * pixel_size - camera_x;
  int y1 = (r.y + r.height) * pixel_size - camera_y;

  y0 = MinInt(MaxInt(y0, 0), h - 1);
  y1 = MinInt(MaxInt(y1, 0), h - 1);
  x0 = MinInt(MaxInt(x0, 0), w - 1);
  x1 = MinInt(MaxInt(x1, 0), w - 1);
  Color* pixels = GetPixels(*out);

  for (int y = y0; y <= y1; y++) {
    for (int x = x0; x <= x1; x++) {
      pixels[y * w + x] = c;
    }
  }
}

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

void RenderImageCompInput(Image* out, Image buffer, Sim* s, int ncomp,
                          PinDesc* cdesc_list) {
  int w = out->width;
  int y = 0;
  int arrow_w = 6;
  int arrow_h = 5;
  int pin_size = 4;
  Color* pixels = GetPixels(*out);
  int lh = GetFontLineHeight();
  FillImage(out, BLANK);
  Color* buffer_pixels = GetPixels(buffer);
  for (int ic = 0; ic < ncomp; ic++) {
    PinDesc cd = cdesc_list[ic];

    int* inputs = NULL;
    int* outputs = NULL;
    if (s != NULL) {
      inputs = SimGetComponentInputs(s, ic);
      outputs = SimGetComponentOutputs(s, ic);
    }
    int ip = 0;
    int op = 0;
    for (int iconn = 0; iconn < cd.num_conn; iconn++) {
      const char* ctxt = cd.conn[iconn].name;
      ConnType ctype = cd.conn[iconn].type;
      int clen = cd.conn[iconn].len;
      Image img_txt = RenderText(ctxt, WHITE);
      for (int i = 0; i < clen; i++) {
        int yy = y + 4 + 2 * i;
        Color c = MAGENTA;
        if (yy < buffer.height) {
          c = buffer_pixels[yy * buffer.width + 0];
        }
        if (COLOR_EQ(c, BLACK) || COLOR_EQ(c, BLANK)) {
          c = WHITE;
        }
        Color cc = c;
        if (s) {
          int s = 0;
          if (ctype == CONN_OUTPUT) {
            s = outputs[op++];
          } else {
            s = inputs[ip++];
          }
          cc = GetSimuColorOnWire(c, s);
        }
        for (int x = w - pin_size; x < w; x++) {
          float k = cc.a / 255.f;
          cc = (Color){cc.r * k, cc.g * k, cc.b * k, 255};
          pixels[yy * w + x] = cc;
        }
      }

      int xs = 0;
      int th = 6 + 2 * clen;
      int ytxt = y + (th - lh) / 2;
      int xtxt = xs + w - pin_size - 4 - img_txt.width - arrow_w;
      RectangleInt r = {0, 0, img_txt.width, img_txt.height};
      CopyImage(img_txt, r, out, (Vector2Int){xtxt, ytxt});
      int* arrow = ctype == CONN_OUTPUT ? arrow_right : arrow_left;
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
}

static inline Color InvertColor(Color c) {
  return (Color){.r = 255 - c.r, .g = 255 - c.g, .b = 255 - c.b, .a = c.a};
}

static inline Color GetGoodColor(Color target, Color pix) {
  // If it's too close,
  int g1 = ColorToGray(target);
  int g2 = ColorToGray(pix);
  if ((g1 > 128 && g2 > 128) || (g1 <= 128 && g2 <= 128)) {
    return InvertColor(target);
  }
  return target;
}

void RenderImageSimpleRect(Image* out, float pixel_size, int camera_x,
                           int camera_y, RectangleInt r, int ls, double t,
                           Color c) {
  int w = out->width;
  int h = out->height;

  int x0 = r.x * pixel_size - camera_x - ls;
  int y0 = r.y * pixel_size - camera_y - ls;
  int x1 = (r.x + r.width) * pixel_size - camera_x + ls;
  int y1 = (r.y + r.height) * pixel_size - camera_y + ls;

  int y0b = MinInt(MaxInt(y0, 0), h - 1);
  int y1b = MinInt(MaxInt(y1, 0), h - 1);
  int x0b = MinInt(MaxInt(x0, 0), w - 1);
  int x1b = MinInt(MaxInt(x1, 0), w - 1);
  Color lut[] = {
      c,
      c,
  };

  int k = 20 * t;
  k = k & 15;
  k = 16 - k;
  Color* pixels = GetPixels(*out);
  // Upper line
  if (y0 == y0b) {
    for (int x = x0b; x <= x1b; x++) {
      int yref = y0 - 1;
      yref = yref < 0 ? 0 : yref;
      Color ref = pixels[yref * w + x];
      pixels[y0 * w + x] = GetGoodColor(lut[((k + x + y0) >> 3) % 2], ref);
    }
  }
  if (y1 == y1b) {
    for (int x = x0b; x <= x1b; x++) {
      int yref = y1 + 1;
      yref = yref > h - 1 ? h - 1 : yref;
      Color ref = pixels[yref * w + x];
      pixels[y1 * w + x] = GetGoodColor(lut[((k + x + y1) >> 3) % 2], ref);
    }
  }
  if (x0 == x0b) {
    for (int y = y0b; y <= y1b; y++) {
      int xref = x0 - 1;
      xref = xref < 0 ? 0 : xref;
      Color ref = pixels[y * w + xref];
      pixels[y * w + x0] = GetGoodColor(lut[((k + x0 + y) >> 3) % 2], ref);
    }
  }
  if (x1 == x1b) {
    for (int y = y0b; y <= y1b; y++) {
      int xref = x1 + 1;
      xref = xref > w - 1 ? w - 1 : xref;
      Color ref = pixels[y * w + xref];
      pixels[y * w + x1] = GetGoodColor(lut[((k + x1 + y) >> 3) % 2], ref);
    }
  }
}
