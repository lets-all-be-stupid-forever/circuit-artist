#ifndef CA_PAINT_H
#define CA_PAINT_H
#include "brush.h"
#include "hist.h"

/**
 * State of the painting functionality.
 * Manages things like tools and camera.
 *
 * Extra x space in the left (in pixels) for camera constraints.
 * Allows the user to go past the default camera movement constraints.  It's
 * useful when we want to display more stuff at the left/top side of the image,
 * like some text or more images.
 */
typedef struct {
  brush_t brush;          // brush_t management
  Hist h;                 // Handles image changes and version management
  int toolBtn;            // Left or right mouse button being used in the tool.
  v2i toolStart;          // Pixel where tool has started (x pos).
  v2i toolEnd;            // Pixel where tool has started (y pos).
  bool tool_pressed;      // user is holding the mouse down and using the tool.
  int camIdx;             // Index of the zoom value.
  Cam2D cam;              // 2D Camera
  Color fg_color;         // Foreground pen color (ie selected color).
  v2i clipboard_offset;   // Stuff for copy-paste.
  Image clipboard_image;  // Stuff for copy-paste.
  bool resizePressed;     // Whether the corner resizing button is pressed.
  bool resizeHovered;     // Whether the corner resizing button is hovered.
  RectangleInt viewport;  // Region where we draw the main image in the screen.
  bool mouseOnTarget;     // wheter mouse is actually on the target image.
  v2i pixelCursor;        // Current position of the cursor in image pixels
  v2 fPixelCursor;        // position in float precision
  v2i extraView;          // extra x space for camera
  bool xmode;

  int lineToolSize;    // Line size for Line tool.
  int lineToolSep;     // line tool separation size
  int lineKey;         // Line width entered via keyboard. (line tool)
  double lineKeyTime;  // Frame where change was made for multi-digit line w
  RectangleInt activeResizeReg;  // Temporary new size when user is changing
  bool toolWithAlt;              // used in brush/line/bucket
  bool altDown;                  // Whether alt is being pressed in this frame.
  int max_img_size;              // maximum size for an image.
  Texture2D tSidePanel;          // Texture used for the side panel with pins.
  RenderTexture2D t_tmp[MAX_LAYERS];  // Tmp render with black pixels
  RTex2D rtPixelTool;  // temporary texture for pixel tool preview
  bool hidden[MAX_LAYERS];
  int render_mode;
  RectangleInt prev_tool_rect;
  int prev_layer; /* Saves previous selected layer for quick back and forth */
} Paint;

void paint_init(Paint* ca);
void paint_destroy(Paint* ca);
void paint_movement_keys(Paint* ca);
void paint_load_image(Paint* ca, Image img);
void paint_center_camera(Paint* ca);
void paint_set_viewport(Paint* ca, RecI viewport);
void paint_paste_image(Paint* ca, Image img, int r);
void paint_render_texture(Paint* ca, Texture2D sidepanel,
                          RenderTexture2D target);
void paint_new_buffer(Paint* ca);
void paint_set_not_dirty(Paint* ca);
void paint_set_tool(Paint* ca, tool_t tool);
void paint_set_color(Paint* ca, Color color);
void paint_enforce_mouse_on_image_if_need(Paint* ca);
void paint_update_pixel_position(Paint* ca);
void paint_layer_alt(Paint* ca);
void paint_handle_keys(Paint* ca);
void paint_handle_mouse(Paint* ca, bool hit);
void paint_handle_camera_movement(Paint* ca);
void paint_handle_wheel_zoom(Paint* ca);
void paint_ensure_camera_within_bounds(Paint* ca);
void paint_act_sel_fill(Paint* ca);
void paint_act_sel_rot(Paint* ca);
void paint_act_sel_fliph(Paint* ca);
void paint_act_sel_flipv(Paint* ca);
Image paint_get_edit_image(Paint* ca);
Color paint_get_color(Paint* ca);
Image paint_export_sel(Paint* ca);
Image paint_export_buf(Paint* ca);
bool paint_get_is_dirty(Paint* ca);
bool paint_get_has_selection(Paint* ca);
bool paint_get_is_tool_sel_moving(Paint* ca);
int paint_get_line_width(Paint* ca);
void paint_set_line_width(Paint* ca, int lw);
int paint_get_line_sep(Paint* ca);
void paint_set_line_sep(Paint* ca, int sep);
bool paint_get_key_line_width_has_just_changed(Paint* ca);
bool paint_get_mouse_over_sel(Paint* ca);
tool_t paint_get_tool(Paint* ca);
tool_t paint_get_display_tool(Paint* ca);
void paint_pause(Paint* pnt);
v2i paint_get_cursor(Paint* pnt);
v2i paint_get_active_sel_size(Paint* pnt);
int paint_get_zoom_perc(Paint* pnt);
void paint_layer_push(Paint* ca);
void paint_layer_pop(Paint* ca);
void paint_set_layer(Paint* ca, int layer);
int paint_get_num_layers(Paint* ca);

int paint_img_width(Paint* ca);
int paint_img_height(Paint* ca);
bool paint_get_layer_vis(Paint* ca, int layer);
void paint_set_layer_vis(Paint* ca, int layer, bool vis);

#endif
