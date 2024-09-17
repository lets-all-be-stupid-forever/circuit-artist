#ifndef CAPAINT_H
#define CAPAINT_H
#include "brush.h"
#include "hist.h"
#include "img.h"
#include "sim.h"

typedef enum {
  LEFT_BTN,
  RIGHT_BTN,
} MouseBtnType;

// Global mode of the paint ui.
typedef enum {
  // When user is drawing.
  MODE_EDIT,
  // When user has clicked on play but the simulator hasn't finished compiling
  // the circuit.
  MODE_COMPILING,
  // When the circuit is compiled and running.
  MODE_SIMU,
} PaintMode;

// State of the painting functionality.
// Manages things like tools and camera.
typedef struct {
  // Brush management
  Brush brush;
  // Simulation context
  Sim s;
  // Parsed image object (related to the simulation)
  ParsedImage pi;
  // Handles image changes and version management
  Hist h;
  // Pixel (x pos) that should be toggled at the given frame
  // -1 means no pixel is to be toggled.
  int queued_toggled_x;
  // Pixel (y pos) that should be toggled at the given frame.
  // -1 means no pixel is to be toggled.
  int queued_toggled_y;
  // Elapsing time for the simulation.
  float simu_time;
  // Number of ticks for the clock (pixel 0,0).
  float clock_frequency;
  // Left or right mouse button being used in the tool.
  MouseBtnType tool_btn;
  // Pixel where tool has started (x pos).
  int tool_start_x;
  // Pixel where tool has started (y pos).
  int tool_start_y;
  // Pixel where tool is ending (x pos).
  int tool_end_x;
  // Pixel where tool is ending (y pos).
  int tool_end_y;
  // Whether tool is on "active" mode, ie if the user is holding the mouse down
  // and using the tool.
  bool tool_pressed;
  // Line size for Line tool.
  int line_tool_size;
  // Shift of the image on screen in screen pixels (x).
  float camera_x;
  // Shift of the image on screen in screen pixels (y).
  float camera_y;
  // Camera spacing.
  float camera_s;
  // Foreground pen color (ie selected color).
  Color fg_color;
  // Stuff for copy-paste.
  Vector2Int clipboard_offset;
  // Stuff for copy-paste.
  Image clipboard_image;
  // Current use mode (editting, simulating, etc).
  PaintMode mode;
  // Whether the corner resizing button is pressed.
  bool resize_pressed;
  // Whether the corner resizing button is hovered.
  bool resize_hovered;
  // Region where we draw the main image in the screen.
  RectangleInt viewport;
  // temporary image used in rendering of the main image.
  Image tmp_render;
  // Current position of the cursor in image pixels. (x pos)
  int pixel_cursor_x;
  // Current position of the cursor in image pixels. (y pos)
  int pixel_cursor_y;
  // wheter mouse is actually on the target image.
  bool mouse_on_target;
  // pixel position cursor in the image in float precision. (x pos)
  float f_pixel_cursor_x;
  // pixel position cursor in the image in float precision. (y pos)
  float f_pixel_cursor_y;
  // Flag for showing a grid during zoom (parameter).
  bool grid_on_zoom;
  // Index of the zoom value. (the zooms/spacing values are discret).
  int camera_i;
  // Color outside the image.
  Color out_color;
  // Extra x space in the left (in pixels) for camera constraints.
  // Allows the user to go past the default camera movement constraints.
  // It's useful when we want to display more stuff at the left/top side of the
  // image, like some text or more images.
  int extrax;
  // Extra y space in the top of the image (in pixels).
  int extray;
  // Line width entered via keyboard. (line tool)
  int line_key_width;
  // Frame where change was made.
  // It's used to allow the user to type a multi-digit line width.
  double line_key_width_time;
  // Clock speed index. 0 = slower, 5 = faster.
  int clock_speed;
  // Temporary new size when user is changing the size of the image via the
  // small square in the bottom-right corner.
  RectangleInt resize_region;
  // Number of clocks passed during simulation.
  int clock_count;
} Paint;

// Constructor.
void PaintLoad(Paint* ca);

// Destructor.
void PaintUnload(Paint* ca);

// Initializes the drawing image.
void PaintLoadImage(Paint* ca, Image img);

// Centers the camera in the center of the image, and also fixes zoom so that
// whole image is visible. Often used right after loading a new image.
void PaintCenterCamera(Paint* ca);

// Sets the region where the image is drawn, so we update internal parameters
// for identifying mouse and other controls.
void PaintSetViewport(Paint* ca, int x, int y, int w, int h);

// Pastes an image.
void PaintPasteImage(Paint* ca, Image img);

// Starts simulation.
void PaintStartSimu(Paint* ca);

// Stops simulation.
void PaintStopSimu(Paint* ca);

// Updates simulation.
// Called in every frame update whenver the simulation is running.
void PaintUpdateSimu(Paint* ca, float delta);

// Renders the drawing image+tools at the target render texture.
void PaintRender(Paint* ca);

// Creates a new empty drawing buffer. (and throws away old one).
void PaintNewBuffer(Paint* ca);

// Resets dirty flag. Usually called after saving.
void PaintSetNotDirty(Paint* ca);

// Sets active tool.
void PaintSetTool(Paint* ca, ToolType tool);

// Sets foreground color.
void PaintSetColor(Paint* ca, Color color);

// Sets simulation on or off.
void PaintToggleSimu(Paint* ca);

// Will move the mouse position towards the target image region when it's
// necessary. For example, when user is dragging or drawing something close to
// the border.
void PaintEnforceMouseOnImageIfNeed(Paint* ca);

// Updates the internal mouse pixel coordinates for the frame.
void PaintUpdatePixelPosition(Paint* ca);

// Handles keyboard events.
void PaintHandleKeys(Paint* ca);

// Handles mouse events.
void PaintHandleMouse(Paint* ca, bool is_target);

// Handles camera movement via middle button.
void PaintHandleCameraMovement(Paint* ca);

// Handles camera zoom via mouse wheel.
void PaintHandleWheelZoom(Paint* ca);

// Gets the state of toggleable wire under the cursor.
// When there's no wire under (or near) the cursor, it returns -1.
// It's used for displaying the hand-pointing cursor over wires that we can
// toggle/interact.
void PaintGetSimuPixelToggleState(Paint* ca, int* cur_state);

// After moving the camera, this call makes sure the camera goes back to a
// position where there's at least a portion of image being displayed.
void PaintEnsureCameraWithinBounds(Paint* ca);

// Action for filling a selection.
void PaintActSelFill(Paint* ca);

// Action for rotating a selection.
void PaintActSelRot(Paint* ca);

// Action for flipping a selection horizontally.
void PaintActSelFlipH(Paint* ca);

// Action for flipping a selection vertically.
void PaintActSelFlipV(Paint* ca);

// Modify clock speed index.
void PaintSetClockSpeed(Paint* ca, int c);

// Gets current active clock speed.
int PaintGetClockSpeed(Paint* ca);

// Returns a reference to the main editting image.
// Used for example when we want to save the image to the disk.
Image PaintGetEditImage(Paint* ca);

// Returns the active tool selected color.
Color PaintGetColor(Paint* ca);

// Returns a reference to the current selection.
Image PaintGetSelBuffer(Paint* ca);

// Returns whether the current image is dirty (ie there was some kind of
// midification on it).
bool PaintGetIsDirty(Paint* ca);

// Returns whether there's a marquee selection in place.
bool PaintGetHasSelection(Paint* ca);

// Gets the number of NANDs detected after simulation parsing.
// Only relevant during simuation.
int PaintGetNumNands(Paint* ca);

// Returns whether the selection is being moved.
bool PaintGetIsToolSelMoving(Paint* ca);

// Returns the active line width for the line tool.
int PaintGetLineWidth(Paint* ca);

// Returns whether the line width for the line tool has just changed (ie, the
// user is still entering the width size on keyboard).
bool PaintGetKeyLineWidthHasJustChanged(Paint* ca);

// Returns current paint mode. (edition/compiling/simulation)
PaintMode PaintGetMode(Paint* ca);

// Gets status of simulation.
// Allows to see if there was some error during parsing/simulation.
int PaintGetSimuStatus(Paint* ca);

// Returns true if the mouse is over a selection.
// Used for changing the mouse to a "move" cursor when the user has a mouse
// over a selection.
bool PaintGetMouseOverSel(Paint* ca);

// Returns the active drawing tool.
ToolType PaintGetTool(Paint* ca);

#endif
