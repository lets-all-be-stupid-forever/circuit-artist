#ifndef RENDERING_H
#define RENDERING_H
#include "img.h"
#include "sim.h"

typedef enum {
  // Wire enters the image.
  CONN_INPUT,
  // Wire leaves the image.
  CONN_OUTPUT,
} ConnType;

typedef struct {
  // Number of connections.
  // A connection is a group of wires/bits represented by a name.
  int num_conn;
  //. List of connections.
  struct {
    // Input or output connection.
    ConnType type;
    // Number of wires (pins) per connection.
    int len;
    // Name of the connectino
    const char* name;
  } conn[50];
} PinDesc;

// Main arguments for drawing in edit mode.
typedef struct {
  // Target rendered image.
  // Mind that it is rendered in CPU.
  Image out;
  // Input main image buffer (pyramid/multi-resolution)
  Image img[3];
  // Selection image buffer (pyramid/multi-resolution).
  Image sel[3];
  // X offset of the selection.
  int sel_off_x;
  // Y offset of the selection.
  int sel_off_y;
  // Camera spacing: the size of a single image pixel in screen pixels.
  float pixel_size;
  // X camera position.
  int camera_x;
  // Y camera position.
  int camera_y;
  // Tool preview image (brush etc).
  Image tool_img;
  // Tool preview offset x in pixels.
  int tool_off_x;
  // Tool preview offset y in pixels.
  int tool_off_y;
  // A simple pixel preview created for pixel/line drawing.
  bool pixel_preview;
  // X position of the pixel preview.
  int pixel_preview_x;
  // Y position of the pixel preview.
  int pixel_preview_y;
  // Color for the pixel preview.
  Color pixel_preview_color;
  // Color outside the main image.
  Color bg;
  // Flag to draw a pixel grid on high zooms.
  bool grid;
  // Color of the high zoom grid.
  Color grid_color;
} RenderImgCtx;

// Renders main screen in edit mode, with tools and everything.
void RenderImageEdit(RenderImgCtx rc);

// renders an image in the camera space. Used for rendering the wire API stuff
// after rendering the main image.
void RenderImageSimple(Image* out, Image img, float pixel_size, int camera_x,
                       int camera_y, int offx, int offy);

// Renders the dotted selection rectangle.
void RenderImageSelRect(Image* out, float pixel_size, int camera_x,
                        int camera_y, RectangleInt r, int ls, double t);

// Renders a filled rectangle in camera space.
// Used for the resizing handle.
void DrawImageSceneRect(Image* out, float pixel_size, int camera_x,
                        int camera_y, Rectangle r, Color c);

// Draws the wire API in the leftr side of the image.
void RenderImageCompInput(Image* out, Image buffer, Sim* simu, int ncomp,
                          PinDesc* cdesc_list);

// Draws a simple rectangle (only the edges).
void RenderImageSimpleRect(Image* out, float pixel_size, int camera_x,
                           int camera_y, RectangleInt r, int ls, double t,
                           Color c);

#endif
