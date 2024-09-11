#ifndef TILING_H
#define TILING_H
#include "defs.h"
#include "ui.h"

// Descriptor for a frame pattern.
// It's used for drawing the frame of the drawing image and the border of
// windows/modals.
typedef struct {
  Texture2D tex;
  Rectangle frame_left;
  Rectangle frame_up;
  Rectangle frame_down;
  Rectangle frame_right;
  Rectangle corner;
} FramePatternDesc;

// Draws a tiled frame around a rectangle region.
// `s` is the scale of the frame.
void DrawTiledFrame(int s, FramePatternDesc pd, Rectangle inner_content);

// Draws a tiled pattern in the whole screen. The pattern is the `src` region
// within the texture `tex`, which is repeated on screen with scaling of `s`.
// Used for drawing UI background.
void DrawTiledScreen(int s, Texture2D tex, Rectangle src);

// Default hard coded tiled pattern.
void DrawDefaultTiledScreen(Ui* ui);

// Default hard coded frame pattern.
void DrawDefaultTiledFrame(Ui* ui, Rectangle inner_content);

#endif
