
#include "paint.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "clipapi.h"
#include "colors.h"
#include "font.h"
#include "img.h"
#include "msg.h"
#include "rlgl.h"
#include "shaders.h"
#include "sim.h"
#include "ui.h"
#include "utils.h"
#include "wmain.h"

#define IMAGE_MOF 8

enum {
  LEFT_BTN,
  RIGHT_BTN,
};

static inline int mini(int a, int b) { return a < b ? a : b; }
static inline int maxi(int a, int b) { return a > b ? a : b; }

static const int MAX_LINE_WIDTH = 256;
static const float RESIZE_HANDLE_SIZE = 20;
static const double LINE_MODIF_THRESHOLD = 1;

static const float ZOOM_LUT[] = {
    -1,    /*  border */
    0.125, /* 1 screen pixel = 8 image pixels */
    0.25,  /*   */
    0.5,   /*   */
    1.0,   /* 1:1 ratio  */
    2.0,   /*   */
    3.0,   /* 1pix image = 3 screen pix */
    4.0,   /*   */
    5.0,   /*   */
    6.0,   /*   */
    8.0,   /*   */
    12.0,  /*   */
    16.0,  /*   */
    24.0,  /*   */
    32.0,  /*   */
    //    48.0,  /*   */
    // 64.0, /* 1pix image = 64 screen pix*/
    -1, /*  border  */
};

static void paint_get_sel_moving_offset(Paint* ca, int* dx, int* dy) {
  if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
    int cx = ca->toolEnd.x - ca->toolStart.x;
    int cy = ca->toolEnd.y - ca->toolStart.y;
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
    *dx = ca->toolEnd.x - ca->toolStart.x;
    *dy = ca->toolEnd.y - ca->toolStart.y;
  }
}

static RectangleInt paint_crop_rect_in_buffer(Paint* ca, RectangleInt r) {
  int x0 = r.x;
  int x1 = r.x + r.width;
  int y0 = r.y;
  int y1 = r.y + r.height;
  Image buffer = hist_get_active_buffer(&ca->h);
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

/*
 * Creates extra image layers if necessary and then pastes.
 * Handles the scenario where you paste an image that has 2
 * layers but you only have 1 layer.
 * It will create the layers and show a message to the user.
 */
static void paint_paste_and_ensure_layer(Paint* ca, Vector2Int offset, int nl,
                                         Image* img) {
  int h_nl = hist_get_num_layers(&ca->h);
  while (h_nl < nl) {
    /* if there's a selection, need to commit it firtst */
    paint_layer_push(ca);
    msg_add("Created layer to fit pasted image's layers.", 4);
    h_nl = hist_get_num_layers(&ca->h);
  }

  hist_act_paste_image(&ca->h, offset, nl, img);
}

/*
 * The rect is on the active layer, not on the bottom layer.
 */
static RectangleInt paint_make_tool_rect(Paint* ca) {
  if (!ca->tool_pressed) {
    return (RectangleInt){0};
  }
  int ll = hist_get_active_layer_llsp(&ca->h);
  int x0 = fmin(ca->toolStart.x >> ll, ca->toolEnd.x >> ll);
  int y0 = fmin(ca->toolStart.y >> ll, ca->toolEnd.y >> ll);
  int x1 = fmax(ca->toolStart.x >> ll, ca->toolEnd.x >> ll) + 1;
  int y1 = fmax(ca->toolStart.y >> ll, ca->toolEnd.y >> ll) + 1;
  return (RectangleInt){
      .x = x0,
      .y = y0,
      .width = x1 - x0,
      .height = y1 - y0,
  };
}

/*
 * Creates the image preview for the ACTIVE layer.
 * The image is in the active layer, but the offset is in the main image
 * coordinates.
 */
static void paint_make_tool_sub_image(Paint* ca, Image* img, Vector2Int* off) {
  Color c = ca->toolBtn == LEFT_BTN ? ca->fg_color : BLACK;
  Image buffer = hist_get_active_buffer(&ca->h);
  int ll = hist_get_active_layer_llsp(&ca->h);
  if (hist_get_tool(&ca->h) == TOOL_LINE) {
    Vector2Int start = {
        .x = ca->toolStart.x >> ll,
        .y = ca->toolStart.y >> ll,
    };
    RectangleInt img_rect = {
        .x = 0,
        .y = 0,
        .width = buffer.width,
        .height = buffer.height,
    };
    int ls = ca->lineToolSize == 0 ? 1 : ca->lineToolSize;
    int sep = ca->lineToolSep <= 0 ? 1 : ca->lineToolSep;
    bool corner = IsKeyDown(KEY_LEFT_SHIFT);
    bool end_corner = is_control_down();
    Vector2Int layer_off;
    draw_image_line_tool(start, paint_make_tool_rect(ca), img_rect, ls, sep,
                         corner, end_corner, c, img, &layer_off);
    // Ensures here we don't have pieces of buffer outside the main buffer.
    RectangleInt buf_rect = {
        .x = layer_off.x,
        .y = layer_off.y,
        .width = img->width,
        .height = img->height,
    };
    RectangleInt cropped_rect = rect_int_get_collision(buf_rect, img_rect);

    // If intersection is empty, we don't do anything
    if (cropped_rect.width == 0) {
      UnloadImage(*img);
      *img = (Image){0};
      return;
    }

    // If sizes don't match, crops the buffer image.
    if (cropped_rect.width != img->width ||
        cropped_rect.height != img->height) {
      RectangleInt crop_img = {
          .x = cropped_rect.x - layer_off.x,
          .y = cropped_rect.y - layer_off.y,
          .width = cropped_rect.width,
          .height = cropped_rect.height,
      };
      layer_off.x = cropped_rect.x;
      layer_off.y = cropped_rect.y;
      Image tmp = crop_image(*img, crop_img);
      UnloadImage(*img);
      *img = tmp;
    }
    //                                                                                                                                                                                                                                                                                                       HistActBuffer(&ca->h, img, off);
    *off = (Vector2Int){
        .x = layer_off.x << ll,
        .y = layer_off.y << ll,
    };

  } else if (hist_get_tool(&ca->h) == TOOL_BRUSH) {
    Vector2Int layer_off = {0};
    brush_make_image(&ca->brush, c, buffer.width, buffer.height, img,
                     &layer_off);
    *off = (Vector2Int){
        .x = layer_off.x << ll,
        .y = layer_off.y << ll,
    };
  } else {
    *img = (Image){0};
    *off = (Vector2Int){0};
  }
}

static void paint_perform_tool_action(Paint* ca) {
  switch (ca->h.tool) {
    case TOOL_BRUSH:
    case TOOL_LINE: {
      if (!ca->toolWithAlt) {
        Image img = {0};
        Vector2Int off = {0};
        paint_make_tool_sub_image(ca, &img, &off);
        hist_act_buffer(&ca->h, img, off);
      }
      break;
    }
    case TOOL_BUCKET: {
      if (!ca->toolWithAlt) {
        RectangleInt layer_rect = paint_make_tool_rect(ca);
        RectangleInt r = paint_crop_rect_in_buffer(ca, layer_rect);
        Image bkt = {0};
        Vector2Int off = {0};
        Image buffer = hist_get_active_buffer(&ca->h);
        Color c = ca->toolBtn == LEFT_BTN ? ca->fg_color : BLACK;
        if (r.width * r.height != 0) {
          bool force = IsKeyDown(KEY_LEFT_SHIFT);
          draw_image_bucket_tool(buffer, r.x, r.y, r.width, r.height, c, force,
                                 &bkt, &off);
          int ll = hist_get_active_layer_llsp(&ca->h);
          v2i offbot = (v2i){
              off.x << ll,
              off.y << ll,
          };
          if (bkt.width > 0) {
            hist_act_buffer(&ca->h, bkt, offbot);
          }
        }
      }
      break;
    }
    case TOOL_SEL: {
      int ll = hist_get_active_layer_llsp(&ca->h);
      if (paint_get_is_tool_sel_moving(ca)) {
        int dx, dy;
        paint_get_sel_moving_offset(ca, &dx, &dy);
        dx = (dx >> ll) << ll;
        dy = (dy >> ll) << ll;
        hist_act_move_sel(&ca->h, dx, dy);
      } else {
        RectangleInt l_rect = paint_make_tool_rect(ca);
        RectangleInt rect = {
            .x = l_rect.x << ll,
            .y = l_rect.y << ll,
            .width = l_rect.width << ll,
            .height = l_rect.height << ll,
        };
        bool multi_layer = is_control_down();
        hist_act_commit_sel(&ca->h, rect, multi_layer);
      }
      break;
    }
    case TOOL_PICKER:
      break;
  }
}

/*
 * Rests some tool-dependent variables when changing tools.
 */
static void paint_on_tool_change(Paint* ca) {
  ca->lineKey = 0;
  ca->lineKeyTime = -1;
}

/*
 * Returns whether there's a marquee selection in place.
 */
bool paint_get_has_selection(Paint* ca) {
  return hist_get_has_selection(&ca->h);
}

static void paint_commit_sel_if_need(Paint* ca) {
  if (paint_get_has_selection(ca)) {
    paint_perform_tool_action(ca);
  }
}

/*
 * Adds a layer to the top of the image stack.
 */
void paint_layer_push(Paint* ca) {
  Hist* h = &ca->h;
  paint_on_tool_change(ca);
  paint_commit_sel_if_need(ca);
  hist_act_layer_push(h);
}

/*
 * Removes the top layer.
 */
void paint_layer_pop(Paint* ca) {
  Hist* h = &ca->h;
  paint_on_tool_change(ca);
  paint_commit_sel_if_need(ca);
  hist_act_layer_pop(h);
}

/*
 * Returns number of layers in the image.
 */
int paint_get_num_layers(Paint* ca) { return hist_get_num_layers(&ca->h); }

/*
 * Changes active layer.
 * Commits selection if there's any active.
 */
void paint_set_layer(Paint* ca, int layer) {
  paint_set_layer_vis(ca, layer, true);
  int al = hist_get_active_layer(&ca->h);
  if (layer == al) return;
  if (al != layer) ca->prev_layer = al;
  if (layer == hist_get_active_layer(&ca->h)) {
    return;
  }
  paint_set_layer_vis(ca, layer, true);
  paint_on_tool_change(ca);
  paint_commit_sel_if_need(ca);
  hist_set_layer(&ca->h, layer);
}

static void paint_pick_color_under_cursor(Paint* ca) {
  bool ld = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  Image buffer = hist_get_active_buffer(&ca->h);
  int ll = hist_get_active_layer_llsp(&ca->h);
  int px = ca->pixelCursor.x >> ll;
  int py = ca->pixelCursor.y >> ll;
  int w = buffer.width;
  int h = buffer.height;
  int al = hist_get_active_layer(&ca->h);
  if (px >= 0 && py >= 0 && px < w && py < h) {
    if (ld) {
      Color c = BLANK;
      for (int l = al; l >= 0; l--) {
        Color* colors = get_pixels(ca->h.buffer[l]);
        if (c.a < 128) c = colors[py * w + px];
      }
      ca->fg_color = c;
    }
    if (ca->fg_color.a < 128) ca->fg_color = BLACK;
    ca->fg_color.a = 255;
  }
}

static void paint_reset_camera(Paint* ca) {
  ca->camIdx = 7;
  ca->cam.sp = ZOOM_LUT[ca->camIdx];
  ca->cam.off.x = 0;
  ca->cam.off.y = 0;
  paint_center_camera(ca);
}

static void paint_zoom_camera_at(Paint* ca, Vector2 screenpos, int z) {
  // position of the mouse in the image before zoom
  RectangleInt r = ca->viewport;
  float p0x = (screenpos.x - ca->cam.off.x - r.x) / ca->cam.sp;
  float p0y = (screenpos.y - ca->cam.off.y - r.y) / ca->cam.sp;

  if (z < 0) {
    if (ZOOM_LUT[ca->camIdx - 1] > 0) {
      ca->camIdx -= 1;
    }
  } else {
    if (ZOOM_LUT[ca->camIdx + 1] > 0) {
      ca->camIdx += 1;
    }
  }
  ca->cam.sp = ZOOM_LUT[ca->camIdx];

  float cx1 = screenpos.x - p0x * ca->cam.sp - r.x;
  float cy1 = screenpos.y - p0y * ca->cam.sp - r.y;
  ca->cam.off.x = round(cx1);
  ca->cam.off.y = round(cy1);
  paint_ensure_camera_within_bounds(ca);
}

static void paint_on_tool_start(Paint* ca) {
  if (hist_get_tool(&ca->h) == TOOL_BRUSH) {
    if (!ca->toolWithAlt) {
      brush_reset(&ca->brush);
    }
  }

  if (hist_get_tool(&ca->h) == TOOL_SEL) {
    // Duplicates selection when moving with CTRL
    if (is_control_down() && paint_get_is_tool_sel_moving(ca)) {
      hist_act_clone_sel(&ca->h);
    }
  }
}

static RectangleInt paint_find_visible_screen_pixels(Paint* ca, int pad) {
  float x0 = -ca->cam.off.x / ca->cam.sp;
  float y0 = -ca->cam.off.y / ca->cam.sp;
  RectangleInt r = ca->viewport;
  float x1 = x0 + r.width / ca->cam.sp;
  float y1 = y0 + r.height / ca->cam.sp;

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

static Vector2Int paint_get_camera_center_offset(Paint* ca, Image img) {
  RectangleInt roi = paint_find_visible_screen_pixels(ca, 0);
  int cx = roi.x + roi.width / 2;
  int cy = roi.y + roi.height / 2;
  int hw = img.width / 2;
  int hh = img.height / 2;
  return (Vector2Int){
      .x = cx - hw,
      .y = cy - hh,
  };
}

static Vector2Int paint_find_best_offset_for_clipboard(Paint* ca) {
  RectangleInt roi = paint_find_visible_screen_pixels(ca, -1);
  Vector2Int off = ca->clipboard_offset;
  RectangleInt clip_roi = {
      .x = off.x,
      .y = off.y,
      .width = ca->clipboard_image.width,
      .height = ca->clipboard_image.height,

  };
  if (rect_int_check_collision(clip_roi, roi)) {
    return off;
  }
  return paint_get_camera_center_offset(ca, ca->clipboard_image);
}

/* For rendering purposes, you have 2 situations where the tool is replaced
 * by a picker tool: 1) tool_pressed=false AND altDown is true AND
 * (tool=brush,bucket,line) 2) tool_pressed=true AND
 * tool_started_with_alt=true AND (tool=brush,bucket,line)
 */
static bool paint_get_tool_is_picker_in_practice(Paint* ca) {
  tool_t tool = hist_get_tool(&ca->h);
  if ((tool == TOOL_LINE || tool == TOOL_BRUSH || tool == TOOL_BUCKET)) {
    if (!ca->tool_pressed && ca->altDown) return true;
    if (ca->tool_pressed && ca->toolWithAlt) return true;
  }
  return false;
}

/*
 * Ensures the camera offset is within "view" bounds.
 *
 * For example, let's say we move the camera too far to the left, this function
 * will shift it back so that there's at least a bit of image in the screen.
 *
 * sz = Size of the paint canvas in pixels.
 * viewport = position of the viewport in the screen.
 * extra = extended padding size of the canvas in X and Y.
 */
static void cam_ensure_camera_bounds(Cam2D* cam, v2i sz, RecI viewport,
                                     v2i extra) {
  float eiw = (extra.x) * cam->sp;
  float eih = (extra.y) * cam->sp;
  // Total image size in scaled pixels
  float iw = (sz.x + extra.x) * cam->sp;
  float ih = (sz.y + extra.y) * cam->sp;
  int sw = viewport.width;
  int sh = viewport.height;
  // I want to have in edge cases either:
  // (i) half screen is void
  // (ii) whole image is visible
  // Mind that `cam.off.x` is the position in window pixels in the screen where
  // I draw the image texture.
  // So drawing image at -iw means it doesnt appear on window.
  float xMax = round(sw - fmin(iw, sw / 2) + eiw);
  float xMin = round(-iw + fmin(iw, sw / 2) + eiw);
  float yMax = round(sh - fmin(ih, sh / 2) + eih);
  float yMin = round(-ih + fmin(ih, sh / 2) + eih);
  cam->off.x = round(fmax(fmin(cam->off.x, xMax), xMin));
  cam->off.y = round(fmax(fmin(cam->off.y, yMax), yMin));
}

static void center_camera(Cam2D* cam, RecI viewport, v2i cvSize) {
  float iw = cvSize.x * cam->sp;
  float ih = cvSize.y * cam->sp;
  int sw = viewport.width;
  int sh = viewport.height;
  float xMax = sw - fmin(iw, sw / 2);
  float xMin = -iw + fmin(iw, sw / 2);
  float yMax = sh - fmin(ih, sh / 2);
  float yMin = -ih + fmin(ih, sh / 2);
  cam->off.x = round((xMax + xMin) / 2);
  cam->off.y = round((yMax + yMin) / 2);
}

static int find_zoom_to_fit(v2i cvSize, RecI viewport) {
  RecI r = viewport;
  int ci = 1;
  while ((ZOOM_LUT[ci + 1] * cvSize.x < r.width) &&
         (ZOOM_LUT[ci + 1] * cvSize.y < r.height) && (ZOOM_LUT[ci + 1] != -1)) {
    ci++;
  }
  return ci;
}

void paint_ensure_camera_within_bounds(Paint* ca) {
  v2i sz = hist_get_buf_size(&ca->h);
  cam_ensure_camera_bounds(&ca->cam, sz, ca->viewport, ca->extraView);
}

// Centers the camera in the center of the image, and also fixes zoom so that
// whole image is visible. Often used right after loading a new image.
void paint_center_camera(Paint* ca) {
  v2i cvSize = hist_get_buf_size(&ca->h);
  ca->camIdx = find_zoom_to_fit(cvSize, ca->viewport);
  ca->cam.sp = ZOOM_LUT[ca->camIdx];
  center_camera(&ca->cam, ca->viewport, cvSize);
}

void paint_init(Paint* ca) {
  *ca = (Paint){0};
  ca->xmode = true;

  if (ui_is_demo()) {
    ca->max_img_size = 64;
  } else {
    // Maximum image size is 8k.
    // For images bigger than that, a dedicated exporter/reader is necessary
    // (stb_image blocks at 8k by default)
    ca->max_img_size = 1 * 8 * 1024;
    // ca->max_img_size = 1024;
  }

  ca->camIdx = 5;
  ca->cam.sp = ZOOM_LUT[ca->camIdx];
  ca->cam.off.x = 0;
  ca->cam.off.y = 0;
  ca->lineToolSize = 1;
  ca->lineToolSep = 1;
  ca->fg_color = WHITE;
  ca->tool_pressed = false;
  hist_init(&ca->h);
  brush_init(&ca->brush);
  ca->clipboard_offset.x = -100000;
  ca->clipboard_offset.y = -100000;
  ca->resizePressed = false;
  ca->resizeHovered = false;
  ca->rtPixelTool = LoadRenderTexture(1, 1);
}

/*
 * Initializes the drawing image.
 * Takes ownership of the image.
 */
void paint_load_image(Paint* ca, Image img) {
  Image tmp[20];
  int nl = -1;
  image_decode_layers(img, &nl, tmp);
  if (nl == 1) {
    tmp[0] = ensure_size_multiple_of(tmp[0], 8);
  }
  if (ui_is_demo()) {
    for (int i = 0; i < nl; i++) {
      image_ensure_max_size(&tmp[i], 64);
    }
  }
  hist_set_buffer(&ca->h, nl, tmp);
  paint_reset_camera(ca);
  paint_ensure_camera_within_bounds(ca);
  UnloadImage(img);
}

// Sets the region where the image is drawn, so we update internal parameters
// for identifying mouse and other controls.
void paint_set_viewport(Paint* ca, RecI viewport) {
  bool need_centering = false;
  if (ca->viewport.width == 0) {
    need_centering = true;
  }
  ca->viewport = viewport;
  if (hist_get_buf_size(&ca->h).x > 0) {
    if (need_centering) {
      paint_center_camera(ca);
    }
    paint_ensure_camera_within_bounds(ca);
  }
}

// Returns the active drawing tool.
tool_t paint_get_tool(Paint* ca) { return hist_get_tool(&ca->h); }

// Pastes an image.
void paint_paste_image(Paint* ca, Image img, int r) {
  Image tmp[20];
  int nl = -1;
  image_decode_layers(img, &nl, tmp);
  for (int i = 0; i < nl; i++) {
    if (r == 1) {
      Image t = tmp[i];
      tmp[i] = rotate_image(tmp[i], 0);
      UnloadImage(t);
    }
    if (r == 3) {
      Image t = tmp[i];
      tmp[i] = rotate_image(tmp[i], 1);
      UnloadImage(t);
    }
    if (r == 2) {
      Image t = tmp[i];
      tmp[i] = rotate_image(tmp[i], 0);
      UnloadImage(t);
      t = tmp[i];
      tmp[i] = rotate_image(tmp[i], 0);
      UnloadImage(t);
    }
  }
  paint_paste_and_ensure_layer(ca, paint_get_camera_center_offset(ca, tmp[0]),
                               nl, tmp);
  UnloadImage(img);
}

// Returns true if the mouse is over a selection.
// Used for changing the mouse to a "move" cursor when the user has a mouse
// over a selection.
bool paint_get_mouse_over_sel(Paint* ca) {
  // Checking if the tool is pressed to avoid scenario where a selection
  // exists and you're creating a new selection and the mouse goes over the
  // existing selection.
  if (!paint_get_has_selection(ca) || ca->tool_pressed) {
    return false;
  }
  int px = ca->pixelCursor.x;
  int py = ca->pixelCursor.y;
  RectangleInt r = hist_get_sel_rect(&ca->h);
  int x0 = r.x;
  int y0 = r.y;
  int x1 = x0 + r.width;
  int y1 = y0 + r.height;
  if (px >= x0 && px < x1 && py >= y0 && py < y1) {
    return true;
  }
  return false;
}

// Returns whether the selection is being moved.
bool paint_get_is_tool_sel_moving(Paint* ca) {
  if (paint_get_has_selection(ca) && ca->tool_pressed) {
    int px = ca->toolStart.x;
    int py = ca->toolStart.y;
    RectangleInt r = hist_get_sel_rect(&ca->h);
    int x0 = r.x;
    int y0 = r.y;
    int x1 = x0 + r.width;
    int y1 = y0 + r.height;
    if (px >= x0 && px < x1 && py >= y0 && py < y1) {
      return true;
    }
  };
  return false;
}

// The tool for to display (example: tool brush might be active but when user
// presses alt, we want to see the tool color picker)
tool_t paint_get_display_tool(Paint* ca) {
  tool_t tool = hist_get_tool(&ca->h);
  if (paint_get_tool_is_picker_in_practice(ca)) {
    tool = TOOL_PICKER;
  }
  return tool;
}

static void paint_draw_resize_handle(Paint* pnt) {
  // Drawing the text in the resize rectangle of the image.
  // I'm drawing here and not in the img because I want to scale the font.
  // When it's dragging, it displays the image size.
  if (pnt->resizeHovered || pnt->resizePressed) {
    RecI vp = pnt->viewport;
    BeginScissorMode(vp.x, vp.y, vp.width, vp.height);
    rlPushMatrix();
    rlTranslatef(vp.x, vp.y, 0);
    rlTranslatef(pnt->cam.off.x, pnt->cam.off.y, 0);
    rlScalef(pnt->cam.sp, pnt->cam.sp, 1);
    v2i sz = hist_get_buf_size(&pnt->h);
    if (pnt->resizePressed) {
      rlTranslatef(pnt->activeResizeReg.width, pnt->activeResizeReg.height, 0);
    } else {
      rlTranslatef(sz.x, sz.y, 0);
    }
    rlScalef(1 / pnt->cam.sp, 1 / pnt->cam.sp, 1);
    rlScalef(2, 2, 1);
    rlTranslatef(10, -14, 0);
    char txt[100];
    if (pnt->resizePressed) {
      int w = pnt->activeResizeReg.width;
      int h = pnt->activeResizeReg.height;
      snprintf(txt, sizeof(txt), "%d x %d", w, h);
    } else {
      snprintf(txt, sizeof(txt), "Drag to resize");
    }
    font_draw_texture(txt, 0, 0, WHITE);
    rlPopMatrix();
    EndScissorMode();
  }
}

static void paint_update_temp_render_texture(Paint* pnt) {
  for (int l = 0; l < hist_get_num_layers(&pnt->h); l++) {
    int tw = pnt->h.t_buffer[l].texture.width;
    int th = pnt->h.t_buffer[l].texture.height;
    int tmp_w = pnt->t_tmp[l].texture.width;
    int tmp_h = pnt->t_tmp[l].texture.height;
    if (tmp_w != tw || tmp_h != th) {
      if (tmp_w > 0) {
        UnloadRenderTexture(pnt->t_tmp[l]);
      }
      pnt->t_tmp[l] = LoadRenderTexture(tw, th);
    }
  }
}

static void paint_draw_rect(float x, float y, float w, int h, Color c) {
  Vector2 position = {0.f, 0.f};
  float rot = 0.f;
  DrawRectanglePro((Rectangle){x, y, w, h}, position, rot, c);
}

static void paint_draw_background(Paint* pnt) {
  Color k = {21, 11, 3, 255};
  // ClearBackground(get_lut_color(COLOR_DARKGRAY));
  ClearBackground(k);
  int tw = pnt->h.t_buffer[0].texture.width;
  int th = pnt->h.t_buffer[0].texture.height;
  float cx = pnt->cam.off.x - 1e-2;
  float cy = pnt->cam.off.y - 1e-2;
  float cs = pnt->cam.sp;
  Color c = {0, 0, 0, 255};
  // Color c = {0, 80, 50, 255};
  paint_draw_rect(cx, cy, tw * cs, th * cs, c);
}

static void paint_draw_sel_rect_line(Rectangle r, double t, Color c) {
  begin_shader(selrect);
  float size[2] = {r.width, r.height};
  float pos[2] = {r.x, r.y};
  int width = 8;
  int shift = 2 * width - ((int)(20 * t)) % (2 * width);
  set_shader_vec2(selrect, rsize, size);
  set_shader_vec2(selrect, rect_pos, pos);
  set_shader_int(selrect, pattern_shift, &shift);
  set_shader_int(selrect, pattern_width, &width);
  Vector2 topLeft = {r.x, r.y};
  Vector2 bottomLeft = {r.x, r.y + r.height};
  Vector2 topRight = {r.x + r.width, r.y};
  Vector2 bottomRight = {r.x + r.width, r.y + r.height};
  rlBegin(RL_QUADS);
  rlNormal3f(0.0f, 0.0f, 1.0f);

  rlColor4ub(c.r, c.g, c.b, c.a);
  rlTexCoord2f(0, 0);
  rlVertex2f(topLeft.x, topLeft.y);
  rlTexCoord2f(0, 1);
  rlVertex2f(bottomLeft.x, bottomLeft.y);
  rlTexCoord2f(1, 1);
  rlVertex2f(bottomRight.x, bottomRight.y);
  rlTexCoord2f(1, 0);
  rlVertex2f(topRight.x, topRight.y);
  rlEnd();
  end_shader();
}

static void paint_draw_sel_rect(float x, float y, float w, int h, double t,
                                Color c) {
  int s = 2;
  Rectangle r_top = {x - s, y - s, w + 2 * s, s};
  Rectangle r_bot = {x - s, y + h, w + 2 * s, s};
  Rectangle r_left = {x - s, y - s, s, h + 2 * s};
  Rectangle r_right = {x + w, y - s, s, h + 2 * s};
  paint_draw_sel_rect_line(r_top, t, c);
  paint_draw_sel_rect_line(r_bot, t, c);
  paint_draw_sel_rect_line(r_left, t, c);
  paint_draw_sel_rect_line(r_right, t, c);
}

/** Draws circuit into a temporary texture containing the complete circuit.
 *
 * Seems a bit overkill, but the main goal is to be able to do zoom out later
 * on.
 */
static void paint_draw_tmp_tex(
    Cam2D cam,   /* camera */
    Tex2D tImg,  /* main image (circuit) */
    Tex2D tSel,  /* selection image */
    Tex2D tTool, /* preview tool (overlay) */
    v2i offSel,  /* offset of selection*/
    v2i offTool, /* offset of tool*/
    v2i tgt,     /* sizeo of the target screen in pixels*/
    Color c) {
  SetTextureFilter(tImg, TEXTURE_FILTER_POINT);
  SetTextureFilter(tSel, TEXTURE_FILTER_POINT);
  int img_size[2] = {tImg.width, tImg.height};
  int sel_size[2] = {tSel.width, tSel.height};
  int tool_size[2] = {tTool.width, tTool.height};
  int sel_off[2] = {offSel.x, offSel.y};
  int tool_off[2] = {offTool.x, offTool.y};
  int tgtW = tgt.x;
  int tgtH = tgt.y;
  float cx = cam.off.x;
  float cy = cam.off.y;
  float cs = cam.sp;
  float fx0 = (-cx) / cs - 8;
  float fy0 = (-cy) / cs - 8;
  float fx1 = (-cx + tgtW) / cs + 8;
  float fy1 = (-cy + tgtH) / cs + 8;
  int ix0 = floorf(fx0);
  int iy0 = floorf(fy0);
  int ix1 = ceilf(fx1);
  int iy1 = ceilf(fy1);
  ix0 = maxi(0, mini(ix0, tImg.width));
  ix1 = maxi(0, mini(ix1, tImg.width));
  iy0 = maxi(0, mini(iy0, tImg.height));
  iy1 = maxi(0, mini(iy1, tImg.height));
  begin_shader(update);
  set_shader_tex(update, sel, tSel);
  set_shader_tex(update, tool, tTool);
  set_shader_ivec2(update, img_size, &img_size);
  set_shader_ivec2(update, sel_off, &sel_off);
  set_shader_ivec2(update, sel_size, &sel_size);
  set_shader_ivec2(update, tool_off, &tool_off);
  set_shader_ivec2(update, tool_size, &tool_size);
  int rx = ix0;
  int ry = iy0;
  int rw = ix1 - ix0 + 1;
  int rh = iy1 - iy0 + 1;
  Rectangle source = {
      .x = (float)rx,
      .y = (float)(tImg.height - (ry + rh)),
      .width = (float)rw,
      .height = (float)-rh,
  };
  Rectangle target = {
      .x = (float)rx,
      .y = (float)ry,
      .width = (float)rw,
      .height = (float)rh,
  };
  DrawTexturePro(tImg, source, target, (Vector2){.x = 0, .y = 0}, 0.0f, c);
  end_shader();
}

// Step2: Draw updated pixels on screen.

static void paint_draw_sidepanel(Cam2D cam, Tex2D tex) {
  rlPushMatrix();
  rlTranslatef(cam.off.x, cam.off.y, 0);
  rlScalef(cam.sp, cam.sp, 1);
  DrawTexture(tex, -tex.width, 0, WHITE);
  rlPopMatrix();
}

static void paint_draw_active_selection_rect(Cam2D cam, RecI rect,
                                             Color color) {
  if (rect.width == 0 || rect.height == 0) {
    return;
  }
  int x0 = rect.x;
  int y0 = rect.y;
  int x1 = rect.x + rect.width;
  int y1 = rect.y + rect.height;
  Rectangle target_rect2 = {
      cam.off.x + x0 * cam.sp - 2,
      cam.off.y + y0 * cam.sp - 2,
      (float)(x1 - x0) * cam.sp + 4,
      (float)(y1 - y0) * cam.sp + 4,
  };
  Rectangle target_rect1 = {
      cam.off.x + x0 * cam.sp - 1,
      cam.off.y + y0 * cam.sp - 1,
      (float)(x1 - x0) * cam.sp + 2,
      (float)(y1 - y0) * cam.sp + 2,
  };
  DrawRectangleLinesEx(target_rect1, 1.0, color);
  DrawRectangleLinesEx(target_rect2, 1.0, color);
}

void make_tool_for_layer(Paint* ca, int layer, v2i* tool_off,
                         RenderTexture2D* rt_tool, bool* tool_is_pixel) {
  *tool_off = (v2i){0};
  *rt_tool = (RenderTexture2D){0};
  *tool_is_pixel = false;
  if (layer != hist_get_active_layer(&ca->h)) {
    return;
  }
  tool_t tool = hist_get_tool(&ca->h);
  if (paint_get_tool_is_picker_in_practice(ca)) {
    tool = TOOL_PICKER;
  }
  *tool_off = (v2i){0};
  *rt_tool = (RenderTexture2D){0};
  if ((tool == TOOL_LINE || tool == TOOL_BRUSH) && ca->tool_pressed) {
    Image tool_img;
    paint_make_tool_sub_image(ca, &tool_img, tool_off);
    if (tool_img.width > 0) {
      *rt_tool = clone_texture_from_image(tool_img);
      UnloadImage(tool_img);
    }
  }
  Color pixel_preview_color;
  *tool_is_pixel = false;
  if ((tool == TOOL_LINE || tool == TOOL_BRUSH) && !ca->tool_pressed &&
      ca->mouseOnTarget) {
    *tool_off = (v2i){
        ca->pixelCursor.x,
        ca->pixelCursor.y,
    };
    *rt_tool = ca->rtPixelTool;
    *tool_is_pixel = true;
    BeginTextureMode(*rt_tool);
    ClearBackground(ca->fg_color);
    EndTextureMode();
  }
}

static Cam2D get_layer_cam(Paint* ca) {
  int ll = hist_get_active_layer_llsp(&ca->h);
  Cam2D cam = ca->cam;
  cam.sp = ca->cam.sp * (1 << ll);
  return cam;
}

/* Projects back into destination */
static void paint_combine_layers(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[1].texture;
  shader_load("pcomb");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  for (int l = 0; l < nl; l++) {
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);
  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_combine_layers_noise(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("layernoise");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){125, 0, 255, 255}),
      ColorNormalize((Color){253, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};
#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    // layer_pat[l] = l == al ? -1 : l;
    layer_pat[l] = l;  // - 1;  //'l == 0 ? -1 : l - 1;
    // layer_pat[l] = l;
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);

    int k = abs(l - al);
    layer_colors[l].w = alut[k];
    // if (k >= 2) layer_mode[l] = -2;
    layer_rad[l] = -abs(l - al) * (spi / 8);
#if 1
    layer_colors[l].x = 0.6;  //* alut[k];
    layer_colors[l].y = 0.6;  //* alut[k];
    layer_colors[l].z = 0.6;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_render_layers_far(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);

  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  for (int l = 0; l < nl; l++) {
    // Texture2D timg = ca->h.t_buffer[l].texture;
    if (ca->hidden[l]) {
      continue;
    }
    Texture2D timg = ca->t_tmp[l].texture;
    int tw = timg.width;
    int th = timg.height;
    Color c = WHITE;
    if (al != l) {
      c.a = 100;
    }
    draw_projection_on_target(ca->cam, timg, (v2i){tw, th}, 0, c);
  }

  EndTextureMode();
}

static void paint_combine_layers_onoff2(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("layeronoff2");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){125, 0, 255, 255}),
      ColorNormalize((Color){253, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  int layer_hidden[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};
#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    layer_hidden[l] = ca->hidden[l];
    // layer_pat[l] = l == al ? -1 : l;
    layer_pat[l] = l;  // - 1;  //'l == 0 ? -1 : l - 1;
    // layer_pat[l] = l;
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);

    int k = abs(l - al);
    layer_colors[l].w = alut[k];
    // if (k >= 2) layer_mode[l] = -2;
    layer_rad[l] = -abs(l - al) * (spi / 8);
#if 1
    layer_colors[l].x = 0.6;  //* alut[k];
    layer_colors[l].y = 0.6;  //* alut[k];
    layer_colors[l].z = 0.6;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_hidden", &layer_hidden[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_combine_layers_onoff(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("layeronoff");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){125, 0, 255, 255}),
      ColorNormalize((Color){253, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};
#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    // layer_pat[l] = l == al ? -1 : l;
    layer_pat[l] = l;  // - 1;  //'l == 0 ? -1 : l - 1;
    // layer_pat[l] = l;
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);

    int k = abs(l - al);
    layer_colors[l].w = alut[k];
    // if (k >= 2) layer_mode[l] = -2;
    layer_rad[l] = -abs(l - al) * (spi / 8);
#if 1
    layer_colors[l].x = 0.6;  //* alut[k];
    layer_colors[l].y = 0.6;  //* alut[k];
    layer_colors[l].z = 0.6;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_combine_layers_pattern(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("pattern_layers");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){125, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
      ColorNormalize((Color){253, 0, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};

#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    layer_pat[l] = l == al ? -1 : l;
    layer_pat[l] = l;
    layer_pat[l] = 5;
    // layer_pat[l] = 6;
    // layer_pat[l] = l;
    // layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);
    // layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);
    layer_mode[0] = 0;  //(xmode ? MODE_FIXED_COLOR : -3);
    // layer_mode[l] = (xmode ? MODE_FIXED_COLOR : -3);
    layer_mode[l] = (xmode ? 0 : -3);

    if (ca->hidden[l]) layer_mode[l] = -3;

    int k = abs(l - al);
    // layer_colors[l].w = alut[k];
    layer_colors[l].w = (l == al) ? 1.0 : 0.5;
    // layer_colors[l].w = 0.3;
    //  if (k >= 2) layer_mode[l] = -2;
    //  layer_rad[l] = -abs(l - al) * (spi / 8);
    //  layer_rad[l] = -l * (spi / 8) - 1;
    layer_rad[l] = -4 * spi / 8 + l * spi / 8;
#if 1
    // layer_colors[l].x = 1.0;  //* alut[k];
    // layer_colors[l].y = 1.0;  //* alut[k];
    // layer_colors[l].z = 1.0;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_comb_custom(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  project_with_dist(ca->cam, ca->h.t_buffer[0].texture, 0);
  EndTextureMode();
}

static void paint_combine_layers_vv(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("layervv");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){0, 255, 0, 255}),
      ColorNormalize((Color){0, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};
#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    layer_pat[l] = l == al ? -1 : l;
    // layer_pat[l] = l;
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);

    int k = abs(l - al);
    layer_colors[l].w = alut[k];
    //  if (k >= 2) layer_mode[l] = -2;
    layer_rad[l] = -abs(l - al) * (spi / 8);
#if 1
    // layer_colors[l].x = 0.6;  //* alut[k];
    // layer_colors[l].y = 0.6;  //* alut[k];
    // layer_colors[l].z = 0.6;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_combine_layers_screen(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("layerscreen");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){0, 255, 0, 255}),
      ColorNormalize((Color){0, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};
#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    layer_pat[l] = l == al ? -1 : l;
    // layer_pat[l] = l;
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);

    int k = abs(l - al);
    layer_colors[l].w = alut[k];
    //  if (k >= 2) layer_mode[l] = -2;
    layer_rad[l] = -abs(l - al) * (spi / 8);
#if 1
    // layer_colors[l].x = 0.6;  //* alut[k];
    // layer_colors[l].y = 0.6;  //* alut[k];
    // layer_colors[l].z = 0.6;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_combine_layers_graymode(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("graymode");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){125, 0, 255, 255}),
      ColorNormalize((Color){253, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};
#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    layer_pat[l] = l == al ? -1 : l;
    // layer_pat[l] = l;
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);

    int k = abs(l - al);
    layer_colors[l].w = alut[k];
    // if (k >= 2) layer_mode[l] = -2;
    layer_rad[l] = -abs(l - al) * (spi / 8);
#if 1
    layer_colors[l].x = 0.6;  //* alut[k];
    layer_colors[l].y = 0.6;  //* alut[k];
    layer_colors[l].z = 0.6;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void paint_combine_layers_shademode(Paint* ca, RenderTexture2D target) {
  BeginTextureMode(target);
  Tex2D tmp = ca->t_tmp[0].texture;
  shader_load("shademode");
  Vector2 sp = {
      (float)ca->cam.sp,
      (float)ca->cam.sp,
  };
  int nl = hist_get_num_layers(&ca->h);
  int al = hist_get_active_layer(&ca->h);
  /* pattern definitions */
  Texture sprites = ui_get_sprites();
  shader_tex("pat_tex", sprites);
  shader_intv("nl", &nl, 1);
  shader_intv("al", &al, 1);
  Vector4 layer_colors[4] = {
      ColorNormalize((Color){255, 0, 0, 255}),
      ColorNormalize((Color){125, 0, 255, 255}),
      ColorNormalize((Color){253, 0, 255, 255}),
      ColorNormalize((Color){0, 255, 255, 255}),
  };
  int layer_pat[4] = {0};
  int layer_rad[4] = {0};
  int spi = (int)(sp.x);
  /*
   * mode=0 same color as normal.
   * mode=1 constant color (provided by layer_color)
   * mode=2 grayscaled-transformed color.
   */
  int layer_mode[4] = {0};
  // int alut[] = {255, 128, 64, 32};
  float alut[] = {1.0, 0.6, 0.3, 0.15};
#define MODE_FIXED_COLOR 1
#define REGULAR_COLOR 0
#define GRAYSCALE 2

  bool xmode = ca->xmode;
#if 1
  for (int l = 0; l < nl; l++) {
    layer_pat[l] = l == al ? -1 : l;
    // layer_pat[l] = l;
    layer_mode[l] = l == al ? 0 : (xmode ? MODE_FIXED_COLOR : -3);

    int k = abs(l - al);
    layer_colors[l].w = alut[k];
    // if (k >= 2) layer_mode[l] = -2;
    layer_rad[l] = -abs(l - al) * (spi / 8);
#if 1
    layer_colors[l].x = 0.6;  //* alut[k];
    layer_colors[l].y = 0.6;  //* alut[k];
    layer_colors[l].z = 0.6;  //* alut[k];
#else
    if (l < al - 1) {
      layer_colors[l].x = 0.6;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.2;  //* alut[k];
    } else if (l > al) {
      layer_colors[l].x = 0.2;  //* alut[k];
      layer_colors[l].y = 0.2;  //* alut[k];
      layer_colors[l].z = 0.6;  //* alut[k];
                                // layer_colors[l].w = 0.9;  //* alut[k];
    }

#endif
    // if (l > al) {
    //   layer_colors[l].x = 0.0;
    //   layer_colors[l].y = 0.0;
    //   layer_colors[l].z = 0.0;
    //   layer_colors[l].w = 1.0;
    // }
    shader_tex(TextFormat("layer%d", l), ca->t_tmp[l].texture);
  }
#endif

#if 0
  /* Version with active layer as foreground always */
  shader_tex("layer2", ca->t_tmp[al].texture);
  layer_pat[0] = 1;
  layer_pat[1] = 2;
  layer_pat[2] = -1;
  layer_mode[0] = al > 0 ? 1 : -2;
  layer_mode[1] = al < nl - 1 ? 1 : -2;
  layer_mode[2] = 0;
  layer_colors[0] = (Vector4){.8, .3, .1, .5};
  layer_colors[1] = (Vector4){.1, .3, .8, .5};
  layer_colors[2] = (Vector4){.1, .8, .8, .5};

  layer_rad[0] = -(spi / 4);
  layer_rad[1] = -(spi / 4);
  layer_rad[2] = 0;
  if (al > 0) shader_tex("layer0", ca->t_tmp[al - 1].texture);
  if (al < nl - 1) shader_tex("layer1", ca->t_tmp[al + 1].texture);
#endif
  int pat_size[2] = {sprites.width, sprites.height};
  shader_ivec2("pat_size", &pat_size[0]);
  shader_intv("layer_pat", &layer_pat[0], 4);
  shader_intv("layer_mode", &layer_mode[0], 4);
  shader_intv("layer_rad", &layer_rad[0], 4);
  shader_vec4v("layer_color", &layer_colors[0], 4);

  int pat0[2] = {496, 384};
  shader_ivec2("pattern0", &pat0[0]);

  Texture2D timg = ca->h.t_buffer[0].texture;
  int img_size[2] = {timg.width, timg.height};
  int tgt_size[2] = {target.texture.width, target.texture.height};
  SetTextureFilter(timg, TEXTURE_FILTER_POINT);
  int mw = img_size[0];
  int mh = img_size[1];
  float cx = ca->cam.off.x;
  float cy = ca->cam.off.y;
  float cs = ca->cam.sp;
  shader_vec2("sp", &sp);

  shader_ivec2("img_size", &img_size[0]);
  shader_ivec2("tgt_size", &tgt_size[0]);
  DrawTexturePro(tmp, (Rectangle){0, 0, (float)mw, (float)-mh},
                 (Rectangle){cx, cy, mw * cs, mh * cs}, (Vector2){0, 0}, 0,
                 WHITE);
  shader_unload();
  EndTextureMode();
}

static void dist2outline(RenderTexture rt) {
  shader_load("distoutline");
  int w = rt.texture.width;
  int h = rt.texture.height;
  Vector2 tsize = {w, h};
  // shader_vec2("sp", &sp);
  shader_vec2("tsize", &tsize);
  draw_tex(rt.texture);
  end_shader();
}

void project_depth(Cam2D cam, Texture2D img, float d) {
  shader_load("projectdepth");
  int w = img.width;
  int h = img.height;
  int size[] = {w, h};
  float cx = cam.off.x;
  float cy = cam.off.y;
  float cs = cam.sp;
  Vector2 depth = {d, d};
  shader_vec2("depth", &depth);
  DrawTexturePro(img,                                    /* image to draw */
                 (Rectangle){0, 0, (float)w, (float)-h}, /* source */
                 (Rectangle){cx, cy, w * cs, h * cs},    /* target */
                 (Vector2){0, 0}, 0,                     /* origin/ rot*/
                 WHITE);
  end_shader();
}

void outline(Texture2D img) {
  shader_load("simple_outline");
  int w = img.width;
  int h = img.height;
  int size[] = {w, h};
  Vector2 tsize = {w, h};
  // shader_vec2("sp", &sp);
  shader_vec2("tsize", &tsize);
  draw_tex(img);
  end_shader();
}

// Renders the drawing image+tools at the target render texture.
void paint_render_texture(Paint* ca, Texture2D sidepanel,
                          RenderTexture2D target) {
  paint_update_temp_render_texture(ca);
  tool_t tool = hist_get_tool(&ca->h);
  if (paint_get_tool_is_picker_in_practice(ca)) {
    tool = TOOL_PICKER;
  }
  v2i seloff = ca->h.seloff;
  if (ca->tool_pressed && paint_get_is_tool_sel_moving(ca)) {
    int dx, dy;
    paint_get_sel_moving_offset(ca, &dx, &dy);
    int ll = hist_get_active_layer_llsp(&ca->h);
    seloff.x += (dx >> ll) << ll;
    seloff.y += (dy >> ll) << ll;
  }
  Paint* pnt = ca;

  // Draws on target texture
  BeginTextureMode(target);
  paint_draw_background(pnt);
  // paint_draw_sidepanel(pnt->cam, sidepanel);
  EndTextureMode();

  int tw = target.texture.width;
  int th = target.texture.height;

  Hist* h = &pnt->h;
  int nl = hist_get_num_layers(h);
#if 1
  for (int kl = 0; kl < nl; kl++) {
    int l = kl;
    // if (hist_get_active_layer(&pnt->h) == 0) {
    //   l = nl - l - 1;
    // }
    RenderTexture rt_tool = {0};
    bool tool_is_pixel = false;
    v2i tool_off = {0};
    make_tool_for_layer(ca, l, &tool_off, &rt_tool, &tool_is_pixel);
    int ll = ca->h.llsp[l];
    v2i l_tool_off = {
        .x = tool_off.x >> ll,
        .y = tool_off.y >> ll,
    };
    /* Draws complete circuit into temporary texture */
    BeginTextureMode(pnt->t_tmp[l]);
    ClearBackground(BLANK);
    v2i szTgt = (v2i){target.texture.width, target.texture.height};
    Color c = WHITE;
    if (l != hist_get_active_layer(&pnt->h)) {
      c.a = 150;
    } else {
      c.a = 225;
    }
    c.a = 255;
    Cam2D cam = pnt->cam;
    cam.sp = pnt->cam.sp * (1 << ll);
    Texture2D t_img = ca->h.t_buffer[l].texture;
    v2i size_img = {t_img.width, t_img.height};
    Texture2D sel = ca->h.t_selbuffer[l].texture;
    v2i l_seloff = {
        seloff.x >> ll,
        seloff.y >> ll,
    };
    paint_draw_tmp_tex(cam, t_img, sel, rt_tool.texture, l_seloff, l_tool_off,
                       szTgt, c);
    EndTextureMode();
    // shutdown
    if (rt_tool.texture.width > 0 && !tool_is_pixel) {
      UnloadRenderTexture(rt_tool);
    }
  }
#endif

  if (ca->cam.sp <= 2.0) {
    paint_render_layers_far(ca, target);
  } else {
    paint_combine_layers_onoff2(ca, target);
  }

  BeginTextureMode(target);
  // ------------------------------------------------------------------------------------
  double curTime = GetTime();
  if (tool == TOOL_SEL && !ca->tool_pressed && hist_get_has_selection(&ca->h)) {
    Cam2D cam = get_layer_cam(ca);
    int ll = hist_get_active_layer_llsp(&ca->h);
    int l = hist_get_active_layer(&ca->h);
    int offx = seloff.x >> ll;
    int offy = seloff.y >> ll;
    Image sel = ca->h.selbuffer[l];
    Color color = WHITE;
    if (l > 0 && ca->h.selbuffer[0].width > 0) {
      color = SKYBLUE;
    }
    paint_draw_sel_rect(cam.off.x + offx * cam.sp, cam.off.y + offy * cam.sp,
                        (float)sel.width * cam.sp, (float)sel.height * cam.sp,
                        curTime, color);
  }

  // ------------------------------------------------------------------------------------
  // Creating the selection
  if (tool == TOOL_SEL && ca->tool_pressed &&
      !paint_get_is_tool_sel_moving(ca)) {
    RectangleInt layer_rect = paint_make_tool_rect(ca);
    RectangleInt rect = paint_crop_rect_in_buffer(ca, layer_rect);
    Cam2D layer_cam = get_layer_cam(ca);
    bool multi = is_control_down();
    paint_draw_active_selection_rect(layer_cam, layer_rect,
                                     multi ? BLUE : GREEN);
  }

  // ------------------------------------------------------------------------------------
  if (tool == TOOL_BUCKET && ca->tool_pressed) {
    RectangleInt layer_rect = paint_make_tool_rect(ca);
    RectangleInt rect = paint_crop_rect_in_buffer(ca, layer_rect);
    Cam2D layer_cam = get_layer_cam(ca);
    paint_draw_active_selection_rect(layer_cam, layer_rect, pnt->fg_color);
  }

  // ------------------------------------------------------------------------------------
  // Mouse selection preview
  if ((tool == TOOL_SEL || tool == TOOL_BUCKET || tool == TOOL_PICKER) &&
      !ca->tool_pressed && !paint_get_mouse_over_sel(ca) && ca->mouseOnTarget) {
    Image buffer = hist_get_active_buffer(&ca->h);
    int ll = hist_get_active_layer_llsp(&ca->h);
    int cx = ca->pixelCursor.x >> ll;
    int cy = ca->pixelCursor.y >> ll;
    if (ca->pixelCursor.x >= 0 && cx < buffer.width && ca->pixelCursor.y >= 0 &&
        cy < buffer.height) {
      int x0 = cx;
      int y0 = cy;
      int x1 = cx + 1;
      int y1 = cy + 1;
      Color c = WHITE;
      Cam2D cam = get_layer_cam(ca);
      Rectangle target_rect1 = {
          cam.off.x + x0 * cam.sp - 1,
          cam.off.y + y0 * cam.sp - 1,
          (float)(x1 - x0) * cam.sp + 2,
          (float)(y1 - y0) * cam.sp + 2,
      };
      DrawRectangleLinesEx(target_rect1, 1.0, c);
    }
  }

  // ------------------------------------------------------------------------------------
  // Resize stuff
  {
    v2i bufsz = hist_get_buf_size(&ca->h);
    Rectangle resize_rect = {
        .x = bufsz.x,
        .y = bufsz.y,
        .width = RESIZE_HANDLE_SIZE / ca->cam.sp,
        .height = RESIZE_HANDLE_SIZE / ca->cam.sp,
    };
    Color c = DARKGRAY;
    Vector2 pixel_pos = {.x = ca->pixelCursor.x, ca->pixelCursor.y};
    int mof = IMAGE_MOF;
    if (ca->resizePressed) {
      int sizex = ca->pixelCursor.x < 8 ? 8 : ca->pixelCursor.x;
      int sizey = ca->pixelCursor.y < 8 ? 8 : ca->pixelCursor.y;
      sizex = mof * ((sizex + mof / 2) / mof);
      sizey = mof * ((sizey + mof / 2) / mof);
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
      ca->activeResizeReg = reg;
      Rectangle target_rect1 = {
          ca->cam.off.x + reg.x * ca->cam.sp - 1,
          ca->cam.off.y + reg.y * ca->cam.sp - 1,
          (float)(reg.width) * ca->cam.sp + 2,
          (float)(reg.height) * ca->cam.sp + 2,
      };
      DrawRectangleLinesEx(target_rect1, 1.0, c);
    } else {
      if (ca->resizeHovered) {
        c = WHITE;
      }
    }
    RectangleInt rect = {
        resize_rect.x,
        resize_rect.y,
        RESIZE_HANDLE_SIZE / ca->cam.sp,
        RESIZE_HANDLE_SIZE / ca->cam.sp,
    };
    Rectangle target_rect1 = {
        ca->cam.off.x + rect.x * ca->cam.sp - 1,
        ca->cam.off.y + rect.y * ca->cam.sp - 1,
        (float)(rect.width) * ca->cam.sp + 2,
        (float)(rect.height) * ca->cam.sp + 2,
    };
    Vector2 position = {0.f, 0.f};
    float rot = 0.f;
    DrawRectanglePro(target_rect1, position, rot, c);
  }

  // Pin connections and text at the side of the image
  paint_draw_resize_handle(pnt);
  EndTextureMode();
}

// Creates a new empty drawing buffer. (and throws away old one).
void paint_new_buffer(Paint* ca) {
  int sz = 256;
  if (ui_is_demo()) {
    sz = 64;
  }
  hist_new_buffer(&ca->h, sz, sz);
  paint_reset_camera(ca);
  ca->prev_layer = 0;
}

// Resets dirty flag. Usually called after saving.
void paint_set_not_dirty(Paint* ca) { hist_set_not_dirty(&ca->h); }

// Sets active tool.
void paint_set_tool(Paint* ca, tool_t tool) {
  tool_t prev = hist_get_tool(&ca->h);
  tool_t next = tool;
  paint_on_tool_change(ca);
  if (prev == next) {
    return;
  }
  if (prev == TOOL_SEL && next != TOOL_SEL && paint_get_has_selection(ca)) {
    paint_perform_tool_action(ca);
    ca->h.hUndo->tool = next;
  }
  hist_set_tool(&ca->h, next);
  ca->tool_pressed = false;
}

// Sets foreground color.
void paint_set_color(Paint* ca, Color color) { ca->fg_color = color; }

void paint_destroy(Paint* ca) {
  brush_destroy(&ca->brush);
  hist_destroy(&ca->h);
  if (ca->tSidePanel.width > 0) {
    UnloadTexture(ca->tSidePanel);
  }
  if (ca->rtPixelTool.id > 0) {
    UnloadRenderTexture(ca->rtPixelTool);
  }
}

static Vector2 project_point_into_rect(Vector2 p, Rectangle r) {
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

// Will move the mouse position towards the target image region when it's
// necessary. For example, when user is dragging or drawing something close to
// the border.
void paint_enforce_mouse_on_image_if_need(Paint* ca) {
  if (ca->tool_pressed) {
    Vector2 pos = GetMousePosition();
    RectangleInt r = ca->viewport;
    Rectangle target_rect = {r.x, r.y, r.width, r.height};
    // Need to project mouse into the rectangle
    Vector2 new_pos = project_point_into_rect(pos, target_rect);
    if (new_pos.x != pos.x || new_pos.y != pos.y) {
      SetMousePosition(new_pos.x, new_pos.y);
      ca->cam.off.x += round(new_pos.x - pos.x);
      ca->cam.off.y += round(new_pos.y - pos.y);
      paint_ensure_camera_within_bounds(ca);
    }
  }
}

// Updates the internal mouse pixel coordinates for the frame.
void paint_update_pixel_position(Paint* ca) {
  Vector2 pos = GetMousePosition();
  RectangleInt r = ca->viewport;
  ca->fPixelCursor.x = (pos.x - ca->cam.off.x - r.x) / ca->cam.sp;
  ca->fPixelCursor.y = (pos.y - ca->cam.off.y - r.y) / ca->cam.sp;
  ca->pixelCursor.x = (int)floorf(ca->fPixelCursor.x);
  ca->pixelCursor.y = (int)floorf(ca->fPixelCursor.y);
  ca->altDown = IsKeyDown(KEY_LEFT_ALT);
  v2i bufsz = hist_get_buf_size(&ca->h);
  Rectangle resize_rect = {
      .x = bufsz.x,
      .y = bufsz.y,
      .width = RESIZE_HANDLE_SIZE / ca->cam.sp,
      .height = RESIZE_HANDLE_SIZE / ca->cam.sp,
  };
  Vector2 pixel_pos = {.x = (float)ca->pixelCursor.x, (float)ca->pixelCursor.y};
  ca->resizeHovered = CheckCollisionPointRec(pixel_pos, resize_rect);
  Rectangle targetRect = {r.x, r.y, r.width, r.height};
  ca->mouseOnTarget = CheckCollisionPointRec(pos, targetRect);
}

// Handles camera zoom via mouse wheel.
void paint_handle_wheel_zoom(Paint* ca) {
  Vector2 pos = GetMousePosition();
  float wheel = GetMouseWheelMove();
  if (fabs(wheel) > 1e-3) {
    int z = wheel > 0 ? 1 : -1;
    paint_zoom_camera_at(ca, pos, z);
    on_click();
  }
}

static void paint_paste_from_clipboard(Paint* ca) {
  // If there's an image in the clipboard, uses that image instead (and save
  // it in internal clipboard)
  Image system_clipboard_image = image_from_clipboard();
  if (system_clipboard_image.width > 0) {
    if (ca->clipboard_image.width > 0) {
      if (ca->clipboard_image.width != system_clipboard_image.width ||
          ca->clipboard_image.height != system_clipboard_image.height) {
        ca->clipboard_offset.x = -100000;
        ca->clipboard_offset.y = -100000;
      }
      UnloadImage(ca->clipboard_image);
    }
    ca->clipboard_image = system_clipboard_image;
  }
  if (ca->clipboard_image.width > 0) {
    Vector2Int offset = paint_find_best_offset_for_clipboard(ca);
    Image tmp[20];
    int nl = -1;
    image_decode_layers(ca->clipboard_image, &nl, tmp);
    paint_paste_and_ensure_layer(ca, offset, nl, tmp);
    UnloadImage(ca->clipboard_image);
    ca->clipboard_image = (Image){0};
  }
}

static void paint_copy_to_clipboard(Paint* ca) {
  if (!paint_get_has_selection(ca)) {
    return;
  }
  // Deletes previous clipboard image/content
  if (ca->clipboard_image.width > 0) {
    UnloadImage(ca->clipboard_image);
    ca->clipboard_image = (Image){0};
  }
  ca->clipboard_offset = hist_get_sel_offset(&ca->h);
  ca->clipboard_image = hist_export_sel(&ca->h);
  // Copies the image in the clipboard as well
  image_to_clipboard(ca->clipboard_image);
}

static int paint_get_number_key_pressed() {
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

static void paint_append_line_width_number(Paint* ca, int key) {
  int old_size = ca->lineKey;
  if (!paint_get_key_line_width_has_just_changed(ca)) {
    old_size = 0;
  }
  int new_size = old_size * 10 + key;
  if (new_size > MAX_LINE_WIDTH) new_size = MAX_LINE_WIDTH;
  if (new_size > 0) {
    ca->lineKey = new_size;
    ca->lineToolSize = new_size;
    ca->lineKeyTime = GetTime();
  }
}

void paint_movement_keys(Paint* ca) {
  // If control is pressed, don't do anything, to avoid conflict with save
  // (Ctrl+S) or other command.
  if (is_control_down()) {
    return;
  }
  int dy = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);
  int dx = IsKeyDown(KEY_A) - IsKeyDown(KEY_D);
  if (dx != 0 || dy != 0) {
    ca->cam.off.x += dx * 10;
    ca->cam.off.y += dy * 10;
    paint_ensure_camera_within_bounds(ca);
  }

  int zoom = IsKeyPressed(KEY_EQUAL) - IsKeyPressed(KEY_MINUS);
  if (zoom != 0) {
    // todo: should be center of view
    RectangleInt r = ca->viewport;
    Vector2 screenpos = {
        .x = r.x + r.width / 2,
        .y = r.y + r.height / 2,
    };
    paint_zoom_camera_at(ca, screenpos, zoom);
    on_click();
  }
}

static void paint_redo(Paint* ca) {
  hist_redo(&ca->h);
  on_click();
  paint_on_tool_change(ca);
  paint_ensure_camera_within_bounds(ca);
}

static void paint_undo(Paint* ca) {
  hist_undo(&ca->h);
  on_click();
  paint_on_tool_change(ca);
  paint_ensure_camera_within_bounds(ca);
}

void paint_handle_keys(Paint* ca) {
  if (IsKeyPressed(KEY_X)) {
    ca->xmode = !ca->xmode;
  }

  if (paint_get_tool(ca) == TOOL_LINE) {
    int key = paint_get_number_key_pressed();
    if (key >= 0) {
      paint_append_line_width_number(ca, key);
      on_click();
    }
  }

  // Undo
  if (is_control_down() && IsKeyPressed(KEY_Z)) {
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
      paint_redo(ca);
    } else {
      paint_undo(ca);
    }
  }

  // Redo
  if (is_control_down() && IsKeyPressed(KEY_Y)) {
    paint_redo(ca);
  }

  // Copy
  if (is_control_down() && IsKeyPressed(KEY_C)) {
    on_click();
    paint_copy_to_clipboard(ca);
  }

  // Paste
  if (is_control_down() && IsKeyPressed(KEY_V)) {
    on_click();
    paint_paste_from_clipboard(ca);
  }

  if (IsKeyPressed(KEY_L)) {
    on_click();
    paint_set_tool(ca, TOOL_LINE);
  }

  if (IsKeyPressed(KEY_B)) {
    on_click();
    paint_set_tool(ca, TOOL_BRUSH);
  }

  if (IsKeyPressed(KEY_G)) {
    on_click();
    paint_set_tool(ca, TOOL_BUCKET);
  }

  if (IsKeyPressed(KEY_I)) {
    on_click();
    paint_set_tool(ca, TOOL_PICKER);
  }

  if (IsKeyPressed(KEY_M)) {
    on_click();
    paint_set_tool(ca, TOOL_SEL);
  }

  // It has multiple selection checks because selection might change
  // between commands
  if (!ca->tool_pressed && paint_get_has_selection(ca) &&
      IsKeyPressed(KEY_ESCAPE)) {
    on_click();
    paint_perform_tool_action(ca);
  }

  if (!ca->tool_pressed) {
    if ((paint_get_has_selection(ca)) &&
        (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
         IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))) {
      // moves the selection...
      int dx = 0;
      int dy = 0;
      // When we press control it moves 2 spaces at a time
      int d = is_control_down() ? 4 : 1;
      int ll = hist_get_active_layer_llsp(&ca->h);
      d = d << ll;
      if (IsKeyPressed(KEY_DOWN)) dy += d;
      if (IsKeyPressed(KEY_UP)) dy -= d;
      if (IsKeyPressed(KEY_LEFT)) dx -= d;
      if (IsKeyPressed(KEY_RIGHT)) dx += d;
      hist_act_move_sel(&ca->h, dx, dy);
      on_click();
    }

    bool has_sel = paint_get_has_selection(ca);
    if (has_sel && IsKeyPressed(KEY_H)) {
      hist_act_flip_sel(&ca->h, ACTION_SEL_FLIP_H);
      on_click();
    }
    if (has_sel && IsKeyPressed(KEY_R)) {
      on_click();
      hist_act_flip_sel(&ca->h, ACTION_SEL_ROTATE);
    }
    if (has_sel && IsKeyPressed(KEY_V) && !is_control_down()) {
      hist_act_flip_sel(&ca->h, ACTION_SEL_FLIP_V);
      on_click();
    }

    if (has_sel && (IsKeyPressed(KEY_F))) {
      hist_act_fill_sel(&ca->h, ca->fg_color);
      on_click();
    }

    if (has_sel && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
      on_click();
      hist_act_delete_sel(&ca->h);
    }
  }
}

/**
 * Stops the resizing process.
 *
 * Called when paint is paused or when mouse is released.
 */
static void Paintool_stop(Paint* pnt) {
  // on_paint_act();
  if (!pnt->tool_pressed) {
    return;
  }
  paint_perform_tool_action(pnt);
  pnt->tool_pressed = false;
}

/**
 * Stops the resizing process.
 *
 * Called when paint is paused or when mouse is released.
 */
static void paint_resizing_stop(Paint* pnt) {
  if (!pnt->resizePressed) {
    return;
  }
  on_click();
  int new_x = pnt->pixelCursor.x;
  int new_y = pnt->pixelCursor.y;
  new_x = new_x < 8 ? 8 : new_x;
  new_y = new_y < 8 ? 8 : new_y;
  int max_size = pnt->max_img_size;
  int mof = IMAGE_MOF;
  new_x = mof * ((new_x + mof / 2) / mof);
  new_y = mof * ((new_y + mof / 2) / mof);
  if (max_size > 0) {
    new_x = new_x > max_size ? max_size : new_x;
    new_y = new_y > max_size ? max_size : new_y;
  }
  v2i bufsz = hist_get_buf_size(&pnt->h);
  int deltax = new_x - bufsz.x;
  int deltay = new_y - bufsz.y;
  hist_act_change_image_size(&pnt->h, deltax, deltay);
  pnt->resizePressed = false;
}

static bool is_rectint_equal(RectangleInt a, RectangleInt b) {
  bool ok = true;
  ok = ok && (a.x == b.x);
  ok = ok && (a.y == b.y);
  ok = ok && (a.width == b.width);
  ok = ok && (a.height == b.height);
  return ok;
}

// Handles mouse events.
void paint_handle_mouse(Paint* ca, bool hit) {
  bool left_pressed = hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  bool right_pressed = hit && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
  bool left_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
  bool right_released = IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
  RectangleInt tool_rect = paint_make_tool_rect(ca);
  if (ca->mouseOnTarget) {
    if (left_pressed) {
      // Resizing feature...
      if (ca->resizeHovered) {
        ca->resizePressed = true;
      } else {
        ca->tool_pressed = !ca->tool_pressed;
        ca->toolWithAlt = ca->altDown;
        ca->toolBtn = LEFT_BTN;
        ca->toolStart.x = ca->pixelCursor.x;
        ca->toolStart.y = ca->pixelCursor.y;
        if (ca->tool_pressed) {
          paint_on_tool_start(ca);
        }
      }
    }

    if (right_pressed) {
      ca->tool_pressed = !ca->tool_pressed;
      ca->toolWithAlt = ca->altDown;
      ca->toolBtn = RIGHT_BTN;
      ca->toolStart = ca->pixelCursor;
      // Cancels resizing when the user clicks on right button
      if (ca->resizePressed) {
        ca->resizePressed = false;
      } else if (ca->tool_pressed) {
        paint_on_tool_start(ca);
      }
    }

    if (ca->tool_pressed) {
      ca->toolEnd = ca->pixelCursor;
      tool_t tool = hist_get_tool(&ca->h);
      if (tool == TOOL_BRUSH && !ca->toolWithAlt) {
        int ll = hist_get_active_layer_llsp(&ca->h);
        int px = ca->pixelCursor.x >> ll;
        int py = ca->pixelCursor.y >> ll;
        if (brush_append_point(&ca->brush, px, py)) {
          on_paint_act();
        };
      }
      bool rect_changed = !is_rectint_equal(tool_rect, ca->prev_tool_rect);
      if (rect_changed) {
        on_paint_act();
      }
      bool bap = false;  // behave as picker
      bap = bap || (tool == TOOL_PICKER);
      bap = bap || ((tool == TOOL_LINE) && (ca->toolWithAlt));
      bap = bap || ((tool == TOOL_BRUSH) && (ca->toolWithAlt));
      bap = bap || ((tool == TOOL_BUCKET) && (ca->toolWithAlt));
      if (bap) {
        on_paint_act();
        paint_pick_color_under_cursor(ca);
      }
    }
  }

  if (left_released || right_released) {
    Paintool_stop(ca);
    paint_resizing_stop(ca);
  }
  ca->prev_tool_rect = tool_rect;
}

// Handles camera movement via middle button.
void paint_handle_camera_movement(Paint* ca) {
  if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
    Vector2 dmouse = GetMouseDelta();
    ca->cam.off.x += dmouse.x;
    ca->cam.off.y += dmouse.y;
    paint_ensure_camera_within_bounds(ca);
  }
}

// Returns whether the current image is dirty (ie there was some kind of
// midification on it).
bool paint_get_is_dirty(Paint* ca) { return hist_get_is_dirty(&ca->h); }

// Returns the active tool selected color.
Color paint_get_color(Paint* ca) { return ca->fg_color; }

// Returns the active line width for the line tool.
int paint_get_line_width(Paint* ca) { return ca->lineToolSize; }
void paint_set_line_width(Paint* ca, int lw) { ca->lineToolSize = lw; }

int paint_get_line_sep(Paint* ca) { return ca->lineToolSep; }
void paint_set_line_sep(Paint* ca, int sep) { ca->lineToolSep = sep; }

// Returns whether the line width for the line tool has just changed (ie, the
// user is still entering the width size on keyboard).
bool paint_get_key_line_width_has_just_changed(Paint* ca) {
  double t = GetTime();
  double last_t = ca->lineKeyTime;
  if (last_t < 0) return false;
  return t - last_t < LINE_MODIF_THRESHOLD;
}

// Action for filling a selection.
void paint_act_sel_fill(Paint* ca) { hist_act_fill_sel(&ca->h, ca->fg_color); }

// Action for rotating a selection.
void paint_act_sel_rot(Paint* ca) {
  hist_act_flip_sel(&ca->h, ACTION_SEL_ROTATE);
}

// Action for flipping a selection horizontally.
void paint_act_sel_fliph(Paint* ca) {
  hist_act_flip_sel(&ca->h, ACTION_SEL_FLIP_H);
}

// Action for flipping a selection vertically.
void paint_act_sel_flipv(Paint* ca) {
  hist_act_flip_sel(&ca->h, ACTION_SEL_FLIP_V);
}

static void update_texture_filter(Paint* ca, Texture2D tex) {
  float s = ca->cam.sp;
  float eps = 1e-3f;
  if (s >= 0.90 - eps) {
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
  } else if (s >= 0.5 - eps) {
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
  } else if (s >= 0.25 - eps) {
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(tex, TEXTURE_FILTER_ANISOTROPIC_4X);
  } else {
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(tex, TEXTURE_FILTER_ANISOTROPIC_8X);
  }
}

void paint_pause(Paint* pnt) {
  pnt->resizeHovered = false;
  Paintool_stop(pnt);
  paint_resizing_stop(pnt);
}

v2i paint_get_cursor(Paint* pnt) { return pnt->pixelCursor; }

// When in edit mode, displays the selection tool active size
v2i paint_get_active_sel_size(Paint* pnt) {
  int tool = paint_get_tool(pnt);
  if (pnt->tool_pressed && !paint_get_is_tool_sel_moving(pnt) &&
      tool == TOOL_SEL) {
    int tx = pnt->toolEnd.x - pnt->toolStart.x;
    int ty = pnt->toolEnd.y - pnt->toolStart.y;
    tx = tx < 0 ? -tx : tx;
    ty = ty < 0 ? -ty : ty;
    return (v2i){tx + 1, ty + 1};
  } else if (paint_get_has_selection(pnt)) {
    RectangleInt r = hist_get_sel_rect(&pnt->h);
    int sw = r.width;
    int sh = r.height;
    return (v2i){sw, sh};
  } else {
    return (v2i){-1, -1};
  }
}

int paint_get_zoom_perc(Paint* pnt) { return 100 * pnt->cam.sp; }

/*
 * Exports an encoded version of the selection.
 */
Image paint_export_sel(Paint* ca) { return hist_export_sel(&ca->h); }

/*
 * Exports an encoded version of the buffer.
 */
Image paint_export_buf(Paint* ca) { return hist_export_buf(&ca->h); }

bool paint_get_layer_vis(Paint* ca, int layer) { return !ca->hidden[layer]; }

void paint_set_layer_vis(Paint* ca, int layer, bool vis) {
  if (layer < hist_get_num_layers(&ca->h)) {
    ca->hidden[layer] = !vis;
  }
}

void paint_layer_alt(Paint* ca) {
  int al = hist_get_active_layer(&ca->h);
  int nl = hist_get_num_layers(&ca->h);
  int nxt = ca->prev_layer;
  if (nxt > nl) nxt = nl - 1;
  if (ca->prev_layer == al) {
    /* If has more than one layer, switches to the upper layer*/
    if (al == 0 && nl > 1) nxt = 1;

    /* If has only one layer, there's nothing to be done */
    if (al == 0 && nl == 1) return;

    if (al > 0 && al < nl - 1) nxt = al + 1;
    if (al > 0 && al == nl - 1) nxt = al - 1;
  }
  paint_set_layer(ca, nxt);
}

int paint_img_width(Paint* ca) { return ca->h.buffer[0].width; }
int paint_img_height(Paint* ca) { return ca->h.buffer[0].height; }
