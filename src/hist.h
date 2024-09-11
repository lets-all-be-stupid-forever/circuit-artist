#ifndef HIST_H
#define HIST_H
#include "defs.h"
#include "img.h"

#define HIST_PYR_LVLS 3

// Active paint tool.
typedef enum {
  TOOL_BRUSH,
  TOOL_SEL,
  TOOL_LINE,
  TOOL_BUCKET,
  TOOL_PICKER,
} ToolType;

// Enum for the type of modifications that can be done in an image.
typedef enum {
  // Creation of a selection (usually via marquee tool).
  ACTION_SEL_CREATE,
  // When we drag an existing selection.
  ACTION_SEL_MOVE,
  // Flipping a selection horizontally
  ACTION_SEL_FLIP_H,
  // Flipping a selection vertically
  ACTION_SEL_FLIP_V,
  // Rotating a selection
  ACTION_SEL_ROTATE,
  // Resizing the image (the small rectangle in the corner)
  ACTION_RESIZE,
  // When a buffer is added to the image, usually through a paste operation or
  // most of the tools like line, brush or text.
  // Indeed the brush tool for example works as "pasting" the result of the
  // brush whenever the user finishes dragging, the same for text, line and
  // bucket tools.
  ACTION_BUFF,
} CmdActionType;

// Command pattern for the image modifications.
// Used for the undo/redo history. Every change to the image pases through a
// command.
typedef struct Cmd {
  // Type of action.
  CmdActionType action_type;
  // For selcreate, the offset of the data_before
  // For line tool, it's the offset of the line rectangle.
  // also the delta for movement selection
  Vector2Int offset;
  // Data before the tool at the target region.
  Image data_before;
  // Data after applying the tool at the target region.
  Image data_after;
  // Rectangle casted on tool for sel create
  RectangleInt sel_rect;
  // if present, instead of selecting from image, uses this image here
  Image paste_data;
  // Used in resizing action.
  Vector2Int resize_delta;
  // Buffer used to store data associated with the resizing of the image at the
  // X side. For example, if the image is reduced, it stores the data that was
  // present in the X side that was erased, so that the action can be undone.
  Image resize_img_delta_x;
  // Same as  resize_img_delta_x but stores the data in the bottom of the image.
  Image resize_img_delta_y;
  // Tool used for the action.
  int tool;
  // Next element in the Linked list.
  struct Cmd* next;
} Cmd;

// Image history change manager.
// This is the owner of the image buffers.
typedef struct {
  // Current active tool
  ToolType tool;
  // Working image buffer (aka real image) stored in a pyramid style.
  // The pyramids are important for zoom-out visualization.
  Image buffer[HIST_PYR_LVLS];
  // Offset of selection image
  Vector2Int seloff;
  // Selection buffer (also stored as pyramid).
  Image selbuffer[HIST_PYR_LVLS];
  // First undo command (linked list)
  Cmd* undo_hist;
  // First redo command (linked list)
  Cmd* redo_hist;
  // whether it has changed. If dirty=false, it means for example that the user
  // can open a new image without being prompted to save.
  bool dirty;
  // maximum size of undo history (after which we will start removing undos)
  int max_undo_hist_size;
} Hist;

// Initializes History change buffers.
void HistLoad(Hist* h);

// Destructor.
void HistUnload(Hist* h);

// Performs an undo operation. (C-Z)
void HistUndo(Hist* h);

// Performs a redo operation. (C-Y)
void HistRedo(Hist* h);

// Resets history and creates an empty image. (New button in the UI)
void HistNewBuffer(Hist* h);

// Dispatchs an action to resize the image.
void HistActChangeImageSize(Hist* h, int deltax, int deltay);

// Dispatches an action to move a marquee selection.
void HistActMoveSel(Hist* h, int dx, int dy);

// Dispatches an action to delete the current marquee selection.
void HistActDeleteSel(Hist* h);

// Dispatches an action to fill the selection (marquee's fill buttton).
void HistActSelFill(Hist* h, Color fill_color);

// Dispatches an action to flip a selection.
// act should be  one of: ACTION_SEL_FLIP_H, ACTION_SEL_FLIP_V or ACTION_SEL_ROTATE.
void HistActSelFlip(Hist* h, CmdActionType act);

// Dispatch an action to paste a new image at a given place (C-V).
void HistActPasteImage(Hist* h, Vector2Int offset, Image img);

// Dispatches an action to apply a buffer.
// Often used with tools like line/brush/text/bucket.
void HistActBuffer(Hist* h, Image img, Vector2Int off);

// Dispatches an action to de-select a selection. (clicking outside, pressing ESC, changing tool etc)
void HistActCommitSel(Hist* h, RectangleInt tool_rect);

// Dispatches an action for duplicating current selection at the beginning of a drag.
// Use case is when we drag a selection with the left CTRL key pressed.
void HistActCloneSel(Hist* h);

// Returns true if there's a region selected.
bool HistGetHasSelection(Hist* h);

// Sets the working buffer of the history.
// Often called at the very start when we just loaded an image from the disk.
void HistSetBuffer(Hist* h, Image buffer);

// Changes the active tool.
// For example, when user changes a tool in the UI.
// Mind though that this is a low-level function, it won't do things like
// commiting selection when the hte user changes from marquee to brush. This
// side is handled elsewhere.
void HistSetTool(Hist* h, ToolType t);

// Returns the active tool.
ToolType HistGetTool(Hist* h);

// Returns the active image buffer as reference (ie original drawing image).
Image HistGetBuffer(Hist* h);

// Returns the active image selection buffer as reference (marquee selection)..
// If there's no selection, an empty image is returned. (as in width=0, height=0, data=NULL)
Image HistGetSelBuffer(Hist* h);

// Returns the offset of the selection.
Vector2Int HistGetSelOffset(Hist* h);

// Getter to whether there was any change in the image.
// Often used when we want to check whether we need to save before opening a
// new image or before exiting the game.
bool HistGetIsDirty(Hist* h);

// Resets the dirty flag.
// Used after we save an image for example.
void HistSetNotDirty(Hist* h);

// Returns whether it's possible to perform and undo.
bool HistGetCanUndo(Hist* h);

// Returns whether it's possible to perform and redo.
bool HistGetCanRedo(Hist* h);

#endif
