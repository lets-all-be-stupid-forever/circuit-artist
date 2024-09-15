#ifndef MINIMAP_H
#define MINIMAP_H
#include "img.h"
#include "paint.h"
#include "ui.h"

// Context for the minimap widget functionality.
typedef struct {
  // Position of the minimap on the screen.
  Rectangle hitbox;
  // border scale
  int s;
  // Rendered minimap image.
  Image img;
  // Texture version
  Texture2D tex;
  // Reference pyramid image (reference).
  Image ref_pyr;
  // An internal counter: it's increased in every call update, and we only
  // update the actual minimap image in a specific frequency like every 30
  // frames, to avoid making it too expensive.
  int cnt;
  // whether mouse is pressed on the widget.
  bool pressed;
  // Rectangle representing current camera on minimap.
  Rectangle cam_rect;
} Minimap;

// Updates minimap state in frame.
void MinimapUpdate(Minimap* m, Paint* ca, Ui* ui, int target_w, int target_h);

// Draws minimap.
void MinimapDraw(Minimap* m);

#endif
