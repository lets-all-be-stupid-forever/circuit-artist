#include "paint.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "api.h"
#include "brush.h"
#include "clip_api.h"
#include "colors.h"
#include "hist.h"
#include "profiler.h"

static const int MAX_LINE_WIDTH = 256;
static const float RESIZE_HANDLE_SIZE = 20;
static const double line_modif_threshold = 1;

static const float zoom_lut[] = {
    -1,  0.2, 0.25, 0.33, 0.5,  1.0,  2.0,  3.0,  4.0, 5.0,
    6.0, 8.0, 12.0, 16.0, 24.0, 32.0, 48.0, 64.0, -1,
};

static void PaintMakeToolSubImage(Paint* ca, Image* img, Vector2Int* off);
static void PaintPickColorUnderCursor(Paint* ca);
static void PaintResetCamera(Paint* ca);
static void PaintZoomCameraAt(Paint* ca, Vector2 screenpos, int z);
static void PaintPasteFromClipboard(Paint* ca);
static void PaintCopyToClipboard(Paint* ca);
static void PaintOnToolStart(Paint* ca);
static void PaintGetSelMovingOffset(Paint* ca, int* dx, int* dy);
static void PaintImageIngress(Paint* ca, Image* img);
static void PaintPerformToolAction(Paint* ca);
static RectangleInt PaintFindVisibleScreenPixels(Paint* ca, int pad);
static RectangleInt PaintCropRectInBuffer(Paint* ca, RectangleInt r);
static RectangleInt PaintMakeToolRect(Paint* ca);
static Vector2Int PaintFindBestOffsetForClipboard(Paint* ca);
static bool PaintGetToolIsPickerInPractice(Paint* ca);

static void PaintOnToolChange(Paint* ca) {
  ca->line_key = 0;
  ca->line_key_time = -1;
}

void PaintPerformToolAction(Paint* ca) {
  switch (ca->h.tool) {
    case TOOL_BRUSH:
    case TOOL_LINE: {
      if (!ca->tool_pressed_with_alt) {
        Image img = {0};
        Vector2Int off = {0};
        PaintMakeToolSubImage(ca, &img, &off);
        HistActBuffer(&ca->h, img, off);
      }
      break;
    }
    case TOOL_BUCKET: {
      if (!ca->tool_pressed_with_alt) {
        RectangleInt r = PaintCropRectInBuffer(ca, PaintMakeToolRect(ca));
        Image bkt = {0};
        Vector2Int off = {0};
        Image buffer = HistGetBuffer(&ca->h);
        Color c = ca->tool_btn == LEFT_BTN ? ca->fg_color : BLACK;
        if (r.width * r.height != 0) {
          DrawImageBucketTool(buffer, r.x, r.y, r.width, r.height, c, &bkt,
                              &off);
          if (bkt.width > 0) {
            HistActBuffer(&ca->h, bkt, off);
          }
        }
      }
      break;
    }
    case TOOL_SEL: {
      if (PaintGetIsToolSelMoving(ca)) {
        int dx, dy;
        PaintGetSelMovingOffset(ca, &dx, &dy);
        HistActMoveSel(&ca->h, dx, dy);
      } else {
        HistActCommitSel(&ca->h, PaintMakeToolRect(ca));
      }
      break;
    }
    case TOOL_PICKER:
      break;
  }
}

RectangleInt PaintCropRectInBuffer(Paint* ca, RectangleInt r) {
  int x0 = r.x;
  int x1 = r.x + r.width;
  int y0 = r.y;
  int y1 = r.y + r.height;
  Image buffer = HistGetBuffer(&ca->h);
  int w = buffer.width;
  int h = buffer.height;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > w) x1 = w;
  if (y1 > h) y1 = h;
  if (x0 > x1) x0 = x1;
  if (y0 > y1) y0 = y1;
  return (RectangleInt){
      .x = x0,
      .y = y0,
      .width = x1 - x0,
      .height = y1 - y0,
  };
}

static RectangleInt PaintFindVisibleScreenPixels(Paint* ca, int pad) {
  float x0 = -ca->camera_x / ca->camera_s;
  float y0 = -ca->camera_y / ca->camera_s;
  RectangleInt r = ca->viewport;
  float x1 = x0 + r.width / ca->camera_s;
  float y1 = y0 + r.height / ca->camera_s;

  // some pixels of padding
  int padding = pad;
  x0 = floorf(x0) - padding;
  y0 = floorf(y0) - padding;
  x1 = ceilf(x1) + padding;
  y1 = ceilf(y1) + padding;

  return (RectangleInt){
      .x = x0,
      .y = y0,
      .width = x1 - x0 + 1,
      .height = y1 - y0 + 1,
  };
}

static Vector2Int PaintGetCameraCenterOffset(Paint* ca, Image img) {
  RectangleInt roi = PaintFindVisibleScreenPixels(ca, 0);
  int cx = roi.x + roi.width / 2;
  int cy = roi.y + roi.height / 2;
  int hw = img.width / 2;
  int hh = img.height / 2;
  return (Vector2Int){
      .x = cx - hw,
      .y = cy - hh,
  };
}

static RectangleInt PaintMakeToolRect(Paint* ca) {
  if (!ca->tool_pressed) {
    return (RectangleInt){0};
  }
  int x0 = fmin(ca->tool_start_x, ca->tool_end_x);
  int y0 = fmin(ca->tool_start_y, ca->tool_end_y);
  int x1 = fmax(ca->tool_start_x, ca->tool_end_x) + 1;
  int y1 = fmax(ca->tool_start_y, ca->tool_end_y) + 1;
  return (RectangleInt){
      .x = x0,
      .y = y0,
      .width = x1 - x0,
      .height = y1 - y0,
  };
}

void PaintEnsureCameraWithinBounds(Paint* ca) {
  Image buffer = HistGetBuffer(&ca->h);
  int extrax = ca->extrax;
  int extray = ca->extray;
  float eiw = (extrax)*ca->camera_s;
  float eih = (extray)*ca->camera_s;
  // Total image size in scaled pixels
  float iw = (buffer.width + extrax) * ca->camera_s;
  float ih = (buffer.height + extray) * ca->camera_s;
  // Physical size in pixels
  // struct region r = ui_get_target_region(C.ui);
  RectangleInt r = ca->viewport;  // ui_get_target_region(C.ui);
  int sw = r.width;
  int sh = r.height;
  // I want to have in edge cases either:
  // (i) half screen is void
  // (ii) whole image is visible
  // Mind that `camera_x` is the position in window pixels in the screen where
  // I draw the image texture.
  // So drawing image at -iw means it doesnt appear on window.
  float x_max = round(sw - fmin(iw, sw / 2) + eiw);
  float x_min = round(-iw + fmin(iw, sw / 2) + eiw);
  float y_max = round(sh - fmin(ih, sh / 2) + eih);
  float y_min = round(-ih + fmin(ih, sh / 2) + eih);
  ca->camera_x = round(fmax(fmin(ca->camera_x, x_max), x_min));
  ca->camera_y = round(fmax(fmin(ca->camera_y, y_max), y_min));
}

void PaintCenterCamera(Paint* ca) {
  Image buffer = HistGetBuffer(&ca->h);
  int ci = 1;
  RectangleInt r = ca->viewport;
  while ((zoom_lut[ci + 1] * buffer.width < r.width) &&
         (zoom_lut[ci + 1] * buffer.height < r.height) &&
         (zoom_lut[ci + 1] != -1)) {
    ci++;
  }
  ca->camera_i = ci;
  ca->camera_s = zoom_lut[ci];
  float iw = buffer.width * ca->camera_s;
  float ih = buffer.height * ca->camera_s;
  int sw = r.width;
  int sh = r.height;
  float x_max = sw - fmin(iw, sw / 2);
  float x_min = -iw + fmin(iw, sw / 2);
  float y_max = sh - fmin(ih, sh / 2);
  float y_min = -ih + fmin(ih, sh / 2);
  ca->camera_x = round((x_max + x_min) / 2);
  ca->camera_y = round((y_max + y_min) / 2);
}

void PaintLoad(Paint* ca, Ui* ui) {
  *ca = (Paint){0};
  if (ui->demo) {
    ca->max_img_size = 256;
  } else {
    // Maximum image size is 8k.
    // For images bigger than that, a dedicated exporter/reader is necessary
    // (stb_image blocks at 8k by default)
    ca->max_img_size = 8 * 1024;
  }

  ca->grid_on_zoom = false;
  ca->camera_i = 5;
  ca->camera_s = zoom_lut[ca->camera_i];
  ca->camera_x = 0;
  ca->camera_y = 0;
  ca->line_tool_size = 1;
  ca->line_tool_sep = 1;
  ca->queued_toggled_x = -1;
  ca->queued_toggled_y = -1;
  ca->fg_color = WHITE;
  ca->clock_frequency = 5000.f;
  ca->mode = MODE_EDIT;
  ca->tool_pressed = false;
  HistLoad(&ca->h);
  BrushLoad(&ca->brush);
  ca->clipboard_offset.x = -100000;
  ca->clipboard_offset.y = -100000;
  ca->resize_pressed = false;
  ca->out_color = GetLutColor(COLOR_DARKGRAY);
  PaintSetClockSpeed(ca, 2);
}

void PaintImageIngress(Paint* ca, Image* img) {
  ImageEnsureMaxSize(img, ca->max_img_size);
  ImageRemoveBlacks(img);
}

void PaintLoadImage(Paint* ca, Image img) {
  PaintStopSimu(ca);
  PaintImageIngress(ca, &img);
  HistSetBuffer(&ca->h, img);
  PaintResetCamera(ca);
  PaintEnsureCameraWithinBounds(ca);
}

void PaintSetViewport(Paint* ca, int x, int y, int w, int h) {
  bool need_centering = false;
  if (ca->viewport.width == 0) {
    need_centering = true;
  }
  ca->viewport = (RectangleInt){x, y, w, h};
  if (HistGetBuffer(&ca->h).width > 0) {
    if (need_centering) {
      PaintCenterCamera(ca);
    }
    PaintEnsureCameraWithinBounds(ca);
  }
}

ToolType PaintGetTool(Paint* ca) { return HistGetTool(&ca->h); }

void PaintPasteImage(Paint* ca, Image img) {
  PaintImageIngress(ca, &img);
  HistActPasteImage(&ca->h, PaintGetCameraCenterOffset(ca, img), img);
}

void PaintStartSimu(Paint* ca) {
  if (ca->mode != MODE_EDIT) {
    return;
  }
  ca->clock_count = 0;
  ca->tool_pressed = false;
  ca->mode = MODE_COMPILING;
  ProfilerTicSingle("ParseImage");
  ca->pi = ParseImage(HistGetBuffer(&ca->h));
  ProfilerTacSingle("ParseImage");
  LevelDesc* cd = ApiGetLevelDesc();
  ProfilerTicSingle("SimLoad");
  SimLoad(&ca->s, ca->pi, cd->num_components, cd->extcomps,
          ApiUpdateLevelComponent, NULL);
  ProfilerTacSingle("SimLoad");
  if (ca->s.status == SIMU_STATUS_OK) {
    ApiStartLevelSimulation();
    SimSimulate(&ca->s, 0);
  }
  ca->simu_time = 0;
  SimGenImage(&ca->s);
  ca->mode = MODE_SIMU;
}

void PaintStopSimu(Paint* ca) {
  if (ca->mode != MODE_SIMU) {
    return;
  }
  if (ca->s.status == SIMU_STATUS_OK) {
    ApiStopLevelSimulation();
  }
  SimUnload(&ca->s);
  UnloadParsedImage(ca->pi);
  ca->mode = MODE_EDIT;
}

void PaintUpdateSimu(Paint* ca, float delta) {
  float clock_time = 1 / ca->clock_frequency;
  ca->simu_time += delta;
  // Part1: Clicks from the user
  if (ca->queued_toggled_x >= 0) {
    SimTogglePixel(&ca->s, ca->queued_toggled_x, ca->queued_toggled_y);
    ca->queued_toggled_x = -1;
    ca->queued_toggled_y = -1;
    // Do I really need to simulate here?
    SimSimulate(&ca->s, 0);
  }

  // Simulation Ticks
  while (ca->simu_time > clock_time) {
    TickResult tr = ApiOnLevelTick(clock_time);
    if (tr.clock_updated) {
      SimDispatchComponent(&ca->s, 0);
      LevelDesc* cd = ApiGetLevelDesc();
      if (tr.clock_value == 1) {
        for (int i = 0; i < cd->num_components; i++) {
          ApiLevelClock(i, SimGetComponentInputs(&ca->s, i), tr.reset);
        }
      }
    }
    ca->clock_count = tr.clock_count;
    SimSimulate(&ca->s, clock_time);
    ca->simu_time -= clock_time;
  }
  ProfilerTic("SimGenImage");
  SimGenImage(&ca->s);
  ProfilerTac();
}

bool PaintGetHasSelection(Paint* ca) { return HistGetHasSelection(&ca->h); }

bool PaintGetMouseOverSel(Paint* ca) {
  // Checking if the tool is pressed to avoid scenario where a selection exists
  // and you're creating a new selection and the mouse goes over the existing
  // selection.
  if (!PaintGetHasSelection(ca) || ca->tool_pressed) {
    return false;
  }
  int px = ca->pixel_cursor_x;
  int py = ca->pixel_cursor_y;
  Image selbuffer = HistGetSelBuffer(&ca->h);
  Vector2Int seloff = HistGetSelOffset(&ca->h);
  int x0 = seloff.x;
  int y0 = seloff.y;
  int x1 = x0 + selbuffer.width;
  int y1 = y0 + selbuffer.height;
  if (px >= x0 && px < x1 && py >= y0 && py < y1) {
    return true;
  }
  return false;
}

bool PaintGetIsToolSelMoving(Paint* ca) {
  if (PaintGetHasSelection(ca) && ca->tool_pressed) {
    int px = ca->tool_start_x;
    int py = ca->tool_start_y;
    Image selbuffer = HistGetSelBuffer(&ca->h);
    Vector2Int seloff = HistGetSelOffset(&ca->h);
    int x0 = seloff.x;
    int y0 = seloff.y;
    int x1 = x0 + selbuffer.width;
    int y1 = y0 + selbuffer.height;
    if (px >= x0 && px < x1 && py >= y0 && py < y1) {
      return true;
    }
  };
  return false;
}

static void PaintMakeToolSubImage(Paint* ca, Image* img, Vector2Int* off) {
  Color c = ca->tool_btn == LEFT_BTN ? ca->fg_color : BLACK;
  Image buffer = HistGetBuffer(&ca->h);
  if (HistGetTool(&ca->h) == TOOL_LINE) {
    Vector2Int start = {
        .x = ca->tool_start_x,
        .y = ca->tool_start_y,
    };
    RectangleInt img_rect = {
        .x = 0,
        .y = 0,
        .width = buffer.width,
        .height = buffer.height,
    };
    int ls = ca->line_tool_size == 0 ? 1 : ca->line_tool_size;
    int sep = ca->line_tool_sep <= 0 ? 1 : ca->line_tool_sep;
    bool corner = IsKeyDown(KEY_LEFT_SHIFT);
    bool end_corner = IsKeyDown(KEY_LEFT_CONTROL);
    DrawImageLineTool(start, PaintMakeToolRect(ca), img_rect, ls, sep, corner,
                      end_corner, c, img, off);
  } else if (HistGetTool(&ca->h) == TOOL_BRUSH) {
    BrushMakeImage(&ca->brush, c, buffer.width, buffer.height, img, off);
  } else {
    *img = (Image){0};
    *off = (Vector2Int){0};
  }
}

static void PaintGetSelMovingOffset(Paint* ca, int* dx, int* dy) {
  if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
    int cx = ca->tool_end_x - ca->tool_start_x;
    int cy = ca->tool_end_y - ca->tool_start_y;
    int acx = cx < 0 ? -cx : cx;
    int acy = cy < 0 ? -cy : cy;
    if (acx > acy) {
      *dx = cx;
      *dy = 0;
    } else {
      *dx = 0;
      *dy = cy;
    }
  } else {
    *dx = ca->tool_end_x - ca->tool_start_x;
    *dy = ca->tool_end_y - ca->tool_start_y;
  }
}

// For rendering purposes, you have 2 situations where the tool is replaced
// by a picker tool: 1) tool_pressed=false AND Alt_down is true AND
// (tool=brush,bucket,line) 2) tool_pressed=true AND
// tool_started_with_alt=true AND (tool=brush,bucket,line)
bool PaintGetToolIsPickerInPractice(Paint* ca) {
  ToolType tool = HistGetTool(&ca->h);
  if ((tool == TOOL_LINE || tool == TOOL_BRUSH || tool == TOOL_BUCKET)) {
    if (!ca->tool_pressed && ca->alt_down) return true;
    if (ca->tool_pressed && ca->tool_pressed_with_alt) return true;
  }
  return false;
}

ToolType PaintGetDisplayTool(Paint* ca) {
  ToolType tool = HistGetTool(&ca->h);
  if (PaintGetToolIsPickerInPractice(ca)) {
    tool = TOOL_PICKER;
  }
  return tool;
}

void PaintRender(Paint* ca) {
  RectangleInt r = ca->viewport;
  int tw = r.width;
  int th = r.height;
  if (ca->tmp_render.width == 0 || ca->tmp_render.width != tw ||
      ca->tmp_render.height != th) {
    if (ca->tmp_render.width > 0) {
      UnloadImage(ca->tmp_render);
    }
    ca->tmp_render = GenImageSimple(tw, th);
  }
  Color bg_color = ca->out_color;
  if (ca->mode == MODE_EDIT || ca->mode == MODE_COMPILING) {
    RenderImgCtx r = {0};
    if (ca->grid_on_zoom && ca->camera_s >= 8) {
      r.grid = true;
      r.grid_color = (Color){40, 40, 40, 255};
    }
    int offx = ca->h.seloff.x;
    int offy = ca->h.seloff.y;
    if (ca->tool_pressed && PaintGetIsToolSelMoving(ca)) {
      int dx, dy;
      PaintGetSelMovingOffset(ca, &dx, &dy);
      offx += dx;
      offy += dy;
    }
    r.sel_off_x = offx;
    r.sel_off_y = offy;
    for (int i = 0; i < 3; i++) {
      r.img[i] = ca->h.buffer[i];
      r.sel[i] = ca->h.selbuffer[i];
    }
    r.out = ca->tmp_render;
    r.pixel_size = ca->camera_s;
    r.camera_x = -ca->camera_x;
    r.camera_y = -ca->camera_y;
    r.bg = bg_color;
    ToolType tool = HistGetTool(&ca->h);
    if (PaintGetToolIsPickerInPractice(ca)) {
      tool = TOOL_PICKER;
    }

    if ((tool == TOOL_LINE || tool == TOOL_BRUSH) && ca->tool_pressed) {
      Vector2Int off;
      PaintMakeToolSubImage(ca, &r.tool_img, &off);
      r.tool_off_x = off.x;
      r.tool_off_y = off.y;
    }
    if ((tool == TOOL_LINE || tool == TOOL_BRUSH) && !ca->tool_pressed &&
        IsCursorOnScreen() && ca->mouse_on_target) {
      r.pixel_preview = true;
      r.pixel_preview_x = ca->pixel_cursor_x;
      r.pixel_preview_y = ca->pixel_cursor_y;
      r.pixel_preview_color = ca->fg_color;
    }
    FillImage(&r.out, bg_color);
    RenderImageEdit(r);

    double t = GetTime();
    if (tool == TOOL_SEL && !ca->tool_pressed && r.sel[0].width > 0) {
      RectangleInt sel_rect = (RectangleInt){
          r.sel_off_x,
          r.sel_off_y,
          r.sel[0].width,
          r.sel[0].height,
      };
      RenderImageSelRect(&ca->tmp_render, r.pixel_size, r.camera_x, r.camera_y,
                         sel_rect, 1, t);
      RenderImageSelRect(&ca->tmp_render, r.pixel_size, r.camera_x, r.camera_y,
                         sel_rect, 2, t);
    }

    // Creating the selection
    if (tool == TOOL_SEL && ca->tool_pressed) {
      if (!PaintGetIsToolSelMoving(ca)) {
        RectangleInt rect = PaintCropRectInBuffer(ca, PaintMakeToolRect(ca));
        if (rect.width > 0 && rect.height > 0) {
          int x0 = rect.x;
          int y0 = rect.y;
          int x1 = rect.x + rect.width;
          int y1 = rect.y + rect.height;
          RectangleInt sel_rect = (RectangleInt){
              x0,
              y0,
              x1 - x0,
              y1 - y0,
          };
          RenderImageSimpleRect(&ca->tmp_render, r.pixel_size, r.camera_x,
                                r.camera_y, sel_rect, 1, t, WHITE);
          RenderImageSimpleRect(&ca->tmp_render, r.pixel_size, r.camera_x,
                                r.camera_y, sel_rect, 2, t, WHITE);
        }
      }
    }

    if (tool == TOOL_BUCKET && ca->tool_pressed) {
      RectangleInt rect = PaintCropRectInBuffer(ca, PaintMakeToolRect(ca));
      if (rect.width > 0 && rect.height > 0) {
        int x0 = rect.x;
        int y0 = rect.y;
        int x1 = rect.x + rect.width;
        int y1 = rect.y + rect.height;
        RectangleInt bucket_rect = (RectangleInt){
            x0,
            y0,
            x1 - x0,
            y1 - y0,
        };
        Color c = ca->fg_color;
        RenderImageSimpleRect(&ca->tmp_render, r.pixel_size, r.camera_x,
                              r.camera_y, bucket_rect, 1, t, c);
        RenderImageSimpleRect(&ca->tmp_render, r.pixel_size, r.camera_x,
                              r.camera_y, bucket_rect, 2, t, c);
      }
    }

    // Mouse selection preview
    if ((tool == TOOL_SEL || tool == TOOL_BUCKET || tool == TOOL_PICKER) &&
        !ca->tool_pressed && IsCursorOnScreen() && !PaintGetMouseOverSel(ca) &&
        ca->mouse_on_target) {
      Image buffer = HistGetBuffer(&ca->h);
      if (ca->pixel_cursor_x >= 0 && ca->pixel_cursor_x < buffer.width &&
          ca->pixel_cursor_y >= 0 && ca->pixel_cursor_y < buffer.height) {
        int x0 = ca->pixel_cursor_x;
        int y0 = ca->pixel_cursor_y;
        int x1 = ca->pixel_cursor_x + 1;
        int y1 = ca->pixel_cursor_y + 1;
        RectangleInt sel_rect = (RectangleInt){
            x0,
            y0,
            x1 - x0,
            y1 - y0,
        };
        Color c = WHITE;
        RenderImageSimpleRect(&ca->tmp_render, r.pixel_size, r.camera_x,
                              r.camera_y, sel_rect, 1, t, c);
      }
    }

    // Resize stuff
    {
      Image buffer = HistGetBuffer(&ca->h);
      Rectangle resize_rect = {
          .x = buffer.width,
          .y = buffer.height,
          .width = RESIZE_HANDLE_SIZE / ca->camera_s,
          .height = RESIZE_HANDLE_SIZE / ca->camera_s,
      };
      Color c = GRAY;
      Vector2 pixel_pos = {.x = ca->pixel_cursor_x, ca->pixel_cursor_y};
      bool is_on_resize_rect = CheckCollisionPointRec(pixel_pos, resize_rect);
      ca->resize_hovered = is_on_resize_rect;
      if (ca->resize_pressed) {
        int sizex = ca->pixel_cursor_x < 8 ? 8 : ca->pixel_cursor_x;
        int sizey = ca->pixel_cursor_y < 8 ? 8 : ca->pixel_cursor_y;
        int max_size = ca->max_img_size;
        if (max_size > 0) {
          sizex = sizex > max_size ? max_size : sizex;
          sizey = sizey > max_size ? max_size : sizey;
        }
        resize_rect.x = sizex;
        resize_rect.y = sizey;
        c = WHITE;
        RectangleInt reg = {
            0,
            0,
            sizex,
            sizey,
        };
        ca->resize_region = reg;
        RenderImageSelRect(&ca->tmp_render, r.pixel_size, r.camera_x,
                           r.camera_y, reg, 1, t);
      } else {
        if (is_on_resize_rect) {
          c = WHITE;
        }
      }
      Rectangle rect = {
          resize_rect.x,
          resize_rect.y,
          RESIZE_HANDLE_SIZE / r.pixel_size,
          RESIZE_HANDLE_SIZE / r.pixel_size,
      };
      DrawImageSceneRect(&ca->tmp_render, r.pixel_size, r.camera_x, r.camera_y,
                         rect, c);
    }
    if (r.tool_img.width > 0) {
      UnloadImage(r.tool_img);
    }
  } else {
    RenderImgCtx r = {0};
    for (int i = 0; i < 3; i++) {
      r.img[i] = ca->s.simulated[i];
    }
    r.out = ca->tmp_render;
    r.pixel_size = ca->camera_s;
    r.camera_x = -ca->camera_x;
    r.camera_y = -ca->camera_y;
    r.bg = bg_color;
    FillImage(&r.out, bg_color);
    RenderImageEdit(r);
  }

  {
    // not sure if this size is ok
    Image tmp = GenImageSimple(200, 800);
    const LevelDesc* cd = ApiGetLevelDesc();
    Sim* s = ca->mode == MODE_EDIT ? NULL : &ca->s;
    RenderImageCompInput(&tmp, HistGetBuffer(&ca->h), s, cd->num_components,
                         cd->pindesc);
    float pixel_size = ca->camera_s;
    int camera_x = -ca->camera_x;
    int camera_y = -ca->camera_y;
    RenderImageSimple(&ca->tmp_render, tmp, pixel_size, camera_x, camera_y,
                      -tmp.width, 0);
    UnloadImage(tmp);
  }
}

void PaintNewBuffer(Paint* ca) {
  PaintStopSimu(ca);
  HistNewBuffer(&ca->h, 256, 256);
  PaintResetCamera(ca);
}

static void PaintResetCamera(Paint* ca) {
  ca->camera_i = 7;
  ca->camera_s = zoom_lut[ca->camera_i];
  ca->camera_x = 0;
  ca->camera_y = 0;
  PaintCenterCamera(ca);
}

Image PaintGetEditImage(Paint* ca) { return HistGetBuffer(&ca->h); }

void PaintSetNotDirty(Paint* ca) { HistSetNotDirty(&ca->h); }

void PaintSetTool(Paint* ca, ToolType tool) {
  ToolType prev = HistGetTool(&ca->h);
  ToolType next = tool;
  PaintOnToolChange(ca);
  if (prev == next) {
    return;
  }
  if (prev == TOOL_SEL && next != TOOL_SEL && PaintGetHasSelection(ca)) {
    PaintPerformToolAction(ca);
    ca->h.undo_hist->tool = next;
  }
  HistSetTool(&ca->h, next);
  ca->tool_pressed = false;
}

void PaintSetColor(Paint* ca, Color color) { ca->fg_color = color; }

void PaintToggleSimu(Paint* ca) {
  if (ca->mode == MODE_EDIT) {
    PaintStartSimu(ca);
  } else {
    PaintStopSimu(ca);
  }
}

void PaintUnload(Paint* ca) {
  BrushUnload(&ca->brush);
  HistUnload(&ca->h);
}

static Vector2 ProjectPointIntoRect(Vector2 p, Rectangle r) {
  float x0 = r.x;
  float y0 = r.y;
  float x1 = r.x + r.width;
  float y1 = r.y + r.height;
  float px = p.x;
  float py = p.y;
  px = px < x0 ? x0 : px;
  px = px > x1 ? x1 : px;
  py = py < y0 ? y0 : py;
  py = py > y1 ? y1 : py;
  return (Vector2){
      .x = px,
      .y = py,
  };
}

void PaintEnforceMouseOnImageIfNeed(Paint* ca) {
  if (ca->tool_pressed) {
    Vector2 pos = GetMousePosition();
    RectangleInt r = ca->viewport;
    Rectangle target_rect = {r.x, r.y, r.width, r.height};
    // Need to project mouse into the rectangle
    Vector2 new_pos = ProjectPointIntoRect(pos, target_rect);
    if (new_pos.x != pos.x || new_pos.y != pos.y) {
      SetMousePosition(new_pos.x, new_pos.y);
      ca->camera_x += round(new_pos.x - pos.x);
      ca->camera_y += round(new_pos.y - pos.y);
      PaintEnsureCameraWithinBounds(ca);
    }
  }
}

void PaintUpdatePixelPosition(Paint* ca) {
  Vector2 pos = GetMousePosition();
  RectangleInt r = ca->viewport;
  ca->f_pixel_cursor_x = (pos.x - ca->camera_x - r.x) / ca->camera_s;
  ca->f_pixel_cursor_y = (pos.y - ca->camera_y - r.y) / ca->camera_s;
  ca->pixel_cursor_x = (int)floorf(ca->f_pixel_cursor_x);
  ca->pixel_cursor_y = (int)floorf(ca->f_pixel_cursor_y);
  ca->alt_down = IsKeyDown(KEY_LEFT_ALT);
}

static void PaintZoomCameraAt(Paint* ca, Vector2 screenpos, int z) {
  // position of the mouse in the image before zoom
  RectangleInt r = ca->viewport;
  float p0x = (screenpos.x - ca->camera_x - r.x) / ca->camera_s;
  float p0y = (screenpos.y - ca->camera_y - r.y) / ca->camera_s;

  if (z < 0) {
    if (zoom_lut[ca->camera_i - 1] > 0) {
      ca->camera_i -= 1;
    }
  } else {
    if (zoom_lut[ca->camera_i + 1] > 0) {
      ca->camera_i += 1;
    }
  }
  ca->camera_s = zoom_lut[ca->camera_i];

  float cx1 = screenpos.x - p0x * ca->camera_s - r.x;
  float cy1 = screenpos.y - p0y * ca->camera_s - r.y;
  ca->camera_x = round(cx1);
  ca->camera_y = round(cy1);
  PaintEnsureCameraWithinBounds(ca);
}

void PaintHandleWheelZoom(Paint* ca) {
  Vector2 pos = GetMousePosition();
  float wheel = GetMouseWheelMove();
  if (fabs(wheel) > 1e-3) {
    int z = wheel > 0 ? 1 : -1;
    PaintZoomCameraAt(ca, pos, z);
  }
}

static Vector2Int PaintFindBestOffsetForClipboard(Paint* ca) {
  RectangleInt roi = PaintFindVisibleScreenPixels(ca, -1);
  Vector2Int off = ca->clipboard_offset;
  RectangleInt clip_roi = {
      .x = off.x,
      .y = off.y,
      .width = ca->clipboard_image.width,
      .height = ca->clipboard_image.height,

  };
  if (CheckRecIntCollision(clip_roi, roi)) {
    return off;
  }
  return PaintGetCameraCenterOffset(ca, ca->clipboard_image);
}

static void PaintPasteFromClipboard(Paint* ca) {
  // If there's an image in the clipboard, uses that image instead (and save it
  // in internal clipboard)
  Image system_clipboard_image = ImageFromClipboard();
  if (system_clipboard_image.width > 0) {
    if (ca->clipboard_image.width > 0) {
      if (ca->clipboard_image.width != system_clipboard_image.width ||
          ca->clipboard_image.height != system_clipboard_image.height) {
        ca->clipboard_offset.x = -100000;
        ca->clipboard_offset.y = -100000;
      }
      UnloadImage(ca->clipboard_image);
    }
    PaintImageIngress(ca, &system_clipboard_image);
    ca->clipboard_image = system_clipboard_image;
  }
  if (ca->clipboard_image.width > 0) {
    Vector2Int offset = PaintFindBestOffsetForClipboard(ca);
    HistActPasteImage(&ca->h, offset, ca->clipboard_image);
  }
}

static void PaintCopyToClipboard(Paint* ca) {
  if (!PaintGetHasSelection(ca)) {
    return;
  }
  // Deletes previous clipboard image/content
  if (ca->clipboard_image.width > 0) {
    UnloadImage(ca->clipboard_image);
    ca->clipboard_image = (Image){0};
  }
  ca->clipboard_offset = HistGetSelOffset(&ca->h);
  ca->clipboard_image = CloneImage(HistGetSelBuffer(&ca->h));
  // Copies the image in the clipboard as well
  ImageToClipboard(ca->clipboard_image);
}

static int GetNumberKeyPressed() {
  if (IsKeyPressed(KEY_ZERO)) return 0;
  if (IsKeyPressed(KEY_ONE)) return 1;
  if (IsKeyPressed(KEY_TWO)) return 2;
  if (IsKeyPressed(KEY_THREE)) return 3;
  if (IsKeyPressed(KEY_FOUR)) return 4;
  if (IsKeyPressed(KEY_FIVE)) return 5;
  if (IsKeyPressed(KEY_SIX)) return 6;
  if (IsKeyPressed(KEY_SEVEN)) return 7;
  if (IsKeyPressed(KEY_EIGHT)) return 8;
  if (IsKeyPressed(KEY_NINE)) return 9;
  return -1;
}

static void PaintAppendLineWidthNumber(Paint* ca, int key) {
  int old_size = ca->line_key;
  if (!PaintGetKeyLineWidthHasJustChanged(ca)) {
    old_size = 0;
  }
  int new_size = old_size * 10 + key;
  if (new_size > MAX_LINE_WIDTH) new_size = MAX_LINE_WIDTH;
  if (new_size > 0) {
    ca->line_key = new_size;
    ca->line_tool_size = new_size;
    ca->line_key_time = GetTime();
  }
}

void PaintHandleKeys(Paint* ca) {
  if (IsKeyPressed(KEY_SPACE)) {
    PaintToggleSimu(ca);
  }
  if (ca->mode == MODE_EDIT) {
    if (PaintGetTool(ca) == TOOL_LINE) {
      int key = GetNumberKeyPressed();
      if (key >= 0) {
        PaintAppendLineWidthNumber(ca, key);
      }
    }

    if (IsKeyPressed(KEY_Z) && IsKeyDown(KEY_LEFT_CONTROL)) {
      HistUndo(&ca->h);
      PaintOnToolChange(ca);
      PaintEnsureCameraWithinBounds(ca);
    }

    if (IsKeyPressed(KEY_Y) && IsKeyDown(KEY_LEFT_CONTROL)) {
      HistRedo(&ca->h);
      PaintOnToolChange(ca);
      PaintEnsureCameraWithinBounds(ca);
    }

    if (IsKeyPressed(KEY_C) && IsKeyDown(KEY_LEFT_CONTROL)) {
      PaintCopyToClipboard(ca);
    }

    if (IsKeyPressed(KEY_V) && IsKeyDown(KEY_LEFT_CONTROL)) {
      PaintPasteFromClipboard(ca);
    }

    if (IsKeyPressed(KEY_L)) {
      PaintSetTool(ca, TOOL_LINE);
    }

    if (IsKeyPressed(KEY_B)) {
      PaintSetTool(ca, TOOL_BRUSH);
    }

    if (IsKeyPressed(KEY_G)) {
      PaintSetTool(ca, TOOL_BUCKET);
    }

    if (IsKeyPressed(KEY_I)) {
      PaintSetTool(ca, TOOL_PICKER);
    }

    if (IsKeyPressed(KEY_M)) {
      PaintSetTool(ca, TOOL_SEL);
    }

    // It has multiple selection checks because selection might change
    // between commands
    if (!ca->tool_pressed && PaintGetHasSelection(ca) &&
        IsKeyPressed(KEY_ESCAPE)) {
      PaintPerformToolAction(ca);
    }

    if (!ca->tool_pressed) {
      if ((PaintGetHasSelection(ca)) &&
          (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
           IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))) {
        // moves the selection...
        int dx = 0;
        int dy = 0;
        // When we press control it moves 2 spaces at a time
        int d = IsKeyDown(KEY_LEFT_CONTROL) ? 4 : 1;
        if (IsKeyPressed(KEY_DOWN)) dy += d;
        if (IsKeyPressed(KEY_UP)) dy -= d;
        if (IsKeyPressed(KEY_LEFT)) dx -= d;
        if (IsKeyPressed(KEY_RIGHT)) dx += d;
        HistActMoveSel(&ca->h, dx, dy);
      }

      bool has_sel = PaintGetHasSelection(ca);
      if (has_sel && IsKeyPressed(KEY_H)) {
        HistActSelFlip(&ca->h, ACTION_SEL_FLIP_H);
      }
      if (has_sel && IsKeyPressed(KEY_R)) {
        HistActSelFlip(&ca->h, ACTION_SEL_ROTATE);
      }
      if (has_sel && IsKeyPressed(KEY_V) && !IsKeyDown(KEY_LEFT_CONTROL)) {
        HistActSelFlip(&ca->h, ACTION_SEL_FLIP_V);
      }

      if (has_sel && (IsKeyPressed(KEY_F))) {
        HistActSelFill(&ca->h, ca->fg_color);
      }

      if (has_sel &&
          (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
        HistActDeleteSel(&ca->h);
      }
    }
  }
}

static void PaintPokeSimulationPixel(Paint* ca) {
  Image buffer = HistGetBuffer(&ca->h);
  if (ca->pixel_cursor_x >= 0 && ca->pixel_cursor_x < buffer.width &&
      ca->pixel_cursor_y >= 0 && ca->pixel_cursor_y < buffer.height) {
    int search_radius = 5;
    SimFindNearestPixelToToggle(&ca->s, search_radius, ca->f_pixel_cursor_x,
                                ca->f_pixel_cursor_y, &ca->queued_toggled_x,
                                &ca->queued_toggled_y);
  }
}

void PaintPickColorUnderCursor(Paint* ca) {
  bool ld = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  int px = ca->pixel_cursor_x;
  int py = ca->pixel_cursor_y;
  Image buffer = HistGetBuffer(&ca->h);
  Color* colors = GetPixels(buffer);
  int w = buffer.width;
  int h = buffer.height;
  if (px >= 0 && py >= 0 && px < w && py < h) {
    if (ld) ca->fg_color = colors[py * w + px];
    if (ca->fg_color.a < 128) ca->fg_color = BLACK;
    ca->fg_color.a = 255;
  }
}

void PaintHandleMouse(Paint* ca, bool is_target) {
  ca->mouse_on_target = is_target;
  if (ca->mode == MODE_EDIT) {
    bool left_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool right_pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    bool left_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    bool right_released = IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
    Vector2 pixel_pos = {.x = ca->pixel_cursor_x, ca->pixel_cursor_y};
    Image buffer = HistGetBuffer(&ca->h);
    Rectangle resize_rect = {
        .x = buffer.width,
        .y = buffer.height,
        .width = RESIZE_HANDLE_SIZE / ca->camera_s,
        .height = RESIZE_HANDLE_SIZE / ca->camera_s,
    };
    bool is_on_resize_rect = CheckCollisionPointRec(pixel_pos, resize_rect);
    if (is_target) {
      if (left_pressed) {
        // Resizing feature...
        if (is_on_resize_rect) {
          ca->resize_pressed = true;
        } else {
          ca->tool_pressed = !ca->tool_pressed;
          ca->tool_pressed_with_alt = ca->alt_down;
          ca->tool_btn = LEFT_BTN;
          ca->tool_start_x = ca->pixel_cursor_x;
          ca->tool_start_y = ca->pixel_cursor_y;
          if (ca->tool_pressed) {
            PaintOnToolStart(ca);
          }
        }
      }

      if (right_pressed) {
        ca->tool_pressed = !ca->tool_pressed;
        ca->tool_pressed_with_alt = ca->alt_down;
        ca->tool_btn = RIGHT_BTN;
        ca->tool_start_x = ca->pixel_cursor_x;
        ca->tool_start_y = ca->pixel_cursor_y;
        // Cancels resizing when the user clicks on right button
        if (ca->resize_pressed) {
          ca->resize_pressed = false;
        } else if (ca->tool_pressed) {
          PaintOnToolStart(ca);
        }
      }

      if (ca->tool_pressed) {
        ca->tool_end_x = ca->pixel_cursor_x;
        ca->tool_end_y = ca->pixel_cursor_y;
        ToolType tool = HistGetTool(&ca->h);
        if (tool == TOOL_BRUSH && !ca->tool_pressed_with_alt) {
          BrushAppendPoint(&ca->brush, ca->pixel_cursor_x, ca->pixel_cursor_y);
        }
        bool bap = false;  // behave as picker
        bap = bap || (tool == TOOL_PICKER);
        bap = bap || ((tool == TOOL_LINE) && (ca->tool_pressed_with_alt));
        bap = bap || ((tool == TOOL_BRUSH) && (ca->tool_pressed_with_alt));
        bap = bap || ((tool == TOOL_BUCKET) && (ca->tool_pressed_with_alt));
        if (bap) {
          PaintPickColorUnderCursor(ca);
        }
      }
    }

    if (left_released || right_released) {
      if (ca->tool_pressed) {
        PaintPerformToolAction(ca);
      }
      if (ca->resize_pressed) {
        int new_x = ca->pixel_cursor_x;
        int new_y = ca->pixel_cursor_y;
        new_x = new_x < 8 ? 8 : new_x;
        new_y = new_y < 8 ? 8 : new_y;

        int max_size = ca->max_img_size;
        if (max_size > 0) {
          new_x = new_x > max_size ? max_size : new_x;
          new_y = new_y > max_size ? max_size : new_y;
        }

        int deltax = new_x - buffer.width;
        int deltay = new_y - buffer.height;
        HistActChangeImageSize(&ca->h, deltax, deltay);
      }
      ca->tool_pressed = false;
      ca->resize_pressed = false;
    }
  } else if (ca->mode == MODE_SIMU && ca->s.status == SIMU_STATUS_OK) {
    if (is_target && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      PaintPokeSimulationPixel(ca);
    }
  }
}

static void PaintOnToolStart(Paint* ca) {
  if (HistGetTool(&ca->h) == TOOL_BRUSH) {
    if (!ca->tool_pressed_with_alt) {
      BrushReset(&ca->brush);
    }
  }

  if (HistGetTool(&ca->h) == TOOL_SEL) {
    // Duplicates selection when moving with CTRL
    if (IsKeyDown(KEY_LEFT_CONTROL) && PaintGetIsToolSelMoving(ca)) {
      HistActCloneSel(&ca->h);
    }
  }
}

void PaintHandleCameraMovement(Paint* ca) {
  if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
    Vector2 dmouse = GetMouseDelta();
    ca->camera_x += dmouse.x;
    ca->camera_y += dmouse.y;
    PaintEnsureCameraWithinBounds(ca);
  }
}

bool PaintGetIsDirty(Paint* ca) { return HistGetIsDirty(&ca->h); }

Image PaintGetSelBuffer(Paint* ca) { return HistGetSelBuffer(&ca->h); }

int PaintGetNumNands(Paint* ca) {
  if (ca->mode == MODE_SIMU) {
    return SimGetNumNands(&ca->s);
  } else {
    return 0;
  }
}

PaintMode PaintGetMode(Paint* ca) { return ca->mode; }

int PaintGetSimuStatus(Paint* ca) {
  if (ca->mode == MODE_SIMU) {
    return ca->s.status;
  } else {
    return -1;
  }
}

Color PaintGetColor(Paint* ca) { return ca->fg_color; }

void PaintGetSimuPixelToggleState(Paint* ca, int* cur_state) {
  Image buffer = HistGetBuffer(&ca->h);
  if (ca->pixel_cursor_x >= 0 && ca->pixel_cursor_x < buffer.width &&
      ca->pixel_cursor_y >= 0 && ca->pixel_cursor_y < buffer.height) {
    int search_radius = 5;
    int qx;
    int qy;
    SimFindNearestPixelToToggle(&ca->s, search_radius, ca->f_pixel_cursor_x,
                                ca->f_pixel_cursor_y, &qx, &qy);
    if (qx != -1) {
      int val = SimGetWireValue(&ca->s, qx, qy);
      *cur_state = val;
    } else {
      *cur_state = -1;
    }
  } else {
    *cur_state = -1;
  }
}

int PaintGetLineWidth(Paint* ca) { return ca->line_tool_size; }
void PaintSetLineWidth(Paint* ca, int lw) { ca->line_tool_size = lw; }

int PaintGetLineSep(Paint* ca) { return ca->line_tool_sep; }
void PaintSetLineSep(Paint* ca, int sep) { ca->line_tool_sep = sep; }

bool PaintGetKeyLineWidthHasJustChanged(Paint* ca) {
  double t = GetTime();
  double last_t = ca->line_key_time;
  if (last_t < 0) return false;
  return t - last_t < line_modif_threshold;
}

void PaintActSelFill(Paint* ca) { HistActSelFill(&ca->h, ca->fg_color); }

void PaintActSelRot(Paint* ca) { HistActSelFlip(&ca->h, ACTION_SEL_ROTATE); }

void PaintActSelFlipH(Paint* ca) { HistActSelFlip(&ca->h, ACTION_SEL_FLIP_H); }

void PaintActSelFlipV(Paint* ca) { HistActSelFlip(&ca->h, ACTION_SEL_FLIP_V); }

void PaintSetClockSpeed(Paint* ca, int c) {
  ca->clock_speed = c;
  double one_hz = 0.5;
  switch (c) {
    case 0:
      ApiSetClockTime(100000);
      break;
    case 1:  // 1hz
      ApiSetClockTime(one_hz);
      break;
    case 2:  // 4hz
      ApiSetClockTime(one_hz / 4.0);
      break;
    case 3:  // 16hz
      ApiSetClockTime(one_hz / 16.0);
      break;
    case 4:  // 64 hz
      ApiSetClockTime(one_hz / 64.0);
      break;
    case 5:  // 1024 hz
      ApiSetClockTime(one_hz / 1024.0);
      break;
  }
}

int PaintGetClockSpeed(Paint* ca) { return ca->clock_speed; }
