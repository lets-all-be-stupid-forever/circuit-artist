#include "cam2d.h"

#include <math.h>

const float CAM2D_ZOOM_LUT[] = {
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
    48.0,  /*   */
    64.0,  /* 1pix image = 64 screen pix*/
    -1,    /*  border  */
};

void cam2d_ensure_bounds(Cam2D* cam, v2i sz, RecI viewport, v2i extra) {
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

void cam2d_center(Cam2D* cam, RecI viewport, v2i sz) {
  float iw = sz.x * cam->sp;
  float ih = sz.y * cam->sp;
  int sw = viewport.width;
  int sh = viewport.height;
  float xMax = sw - fmin(iw, sw / 2);
  float xMin = -iw + fmin(iw, sw / 2);
  float yMax = sh - fmin(ih, sh / 2);
  float yMin = -ih + fmin(ih, sh / 2);
  cam->off.x = round((xMax + xMin) / 2);
  cam->off.y = round((yMax + yMin) / 2);
}

int cam2d_find_zoom_to_fit(v2i sz, RecI viewport) {
  RecI r = viewport;
  int ci = 1;
  while ((CAM2D_ZOOM_LUT[ci + 1] * sz.x < r.width) &&
         (CAM2D_ZOOM_LUT[ci + 1] * sz.y < r.height) &&
         (CAM2D_ZOOM_LUT[ci + 1] != -1)) {
    ci++;
  }
  return ci;
}

void cam2d_zoom_at(Cam2D* cam, int* cam_idx, RecI viewport, v2 screenpos,
                   int z) {
  // position of the mouse in the image before zoom
  RecI r = viewport;
  float p0x = (screenpos.x - cam->off.x - r.x) / cam->sp;
  float p0y = (screenpos.y - cam->off.y - r.y) / cam->sp;

  if (z < 0) {
    if (CAM2D_ZOOM_LUT[*cam_idx - 1] > 0) {
      *cam_idx -= 1;
    }
  } else {
    if (CAM2D_ZOOM_LUT[*cam_idx + 1] > 0) {
      *cam_idx += 1;
    }
  }
  cam->sp = CAM2D_ZOOM_LUT[*cam_idx];

  float cx1 = screenpos.x - p0x * cam->sp - r.x;
  float cy1 = screenpos.y - p0y * cam->sp - r.y;
  cam->off.x = round(cx1);
  cam->off.y = round(cy1);
}

v2 cam2d_screen_to_image(Cam2D cam, RecI viewport, v2 screenpos) {
  return (v2){
      (screenpos.x - cam.off.x - viewport.x) / cam.sp,
      (screenpos.y - cam.off.y - viewport.y) / cam.sp,
  };
}

