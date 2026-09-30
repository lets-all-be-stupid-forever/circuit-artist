#ifndef CA_CAM2D_H
#define CA_CAM2D_H

#include "common.h"

/* Pure Cam2D helpers shared by the paint canvas and the standalone viewer.
 * Zoom levels are discrete: `cam_idx` indexes CAM2D_ZOOM_LUT, and cam->sp is
 * always CAM2D_ZOOM_LUT[cam_idx]. Both ends of the table hold -1 as guards. */

extern const float CAM2D_ZOOM_LUT[];

/* Clamps cam->off so that either half the viewport is void or the whole image
 * is visible. `extra` is additional image-space margin (top/left). */
void cam2d_ensure_bounds(Cam2D* cam, v2i sz, RecI viewport, v2i extra);

/* Centers the image in the viewport (does not change zoom). */
void cam2d_center(Cam2D* cam, RecI viewport, v2i sz);

/* Largest zoom index at which the whole image fits in the viewport. */
int cam2d_find_zoom_to_fit(v2i sz, RecI viewport);

/* Steps the zoom one level (z>0 in, z<0 out) keeping the image point under
 * `screenpos` fixed on screen. Does not clamp: call cam2d_ensure_bounds. */
void cam2d_zoom_at(Cam2D* cam, int* cam_idx, RecI viewport, v2 screenpos,
                   int z);

/* Screen position -> fractional image pixel. */
v2 cam2d_screen_to_image(Cam2D cam, RecI viewport, v2 screenpos);

#endif

