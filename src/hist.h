#ifndef CA_HIST_H
#define CA_HIST_H
#include "common.h"
#include "rectint.h"

/*
 * Active paint tool.
 */
typedef enum {
  TOOL_BRUSH,
  TOOL_SEL,
  TOOL_LINE,
  TOOL_BUCKET,
  TOOL_PICKER,
} tool_t;

/*
 * Enum for the type of modifications that can be done in an image.
 */
typedef enum {
  /*
   * Creation of a selection (usually via marquee tool).
   * The position of a selection is always defined in the base image
   * coordinates.
   */
  ACTION_SEL_CREATE,
  /*
   * When we drag an existing selection.
   */
  ACTION_SEL_MOVE,
  /*
   * Flipsa selection horizontally
   */
  ACTION_SEL_FLIP_H,
  /*
   * Flipps a selection vertically
   */
  ACTION_SEL_FLIP_V,
  /*
   * Rotates a selection clockwise.
   */
  ACTION_SEL_ROTATE,
  /*
   * Resizing the image (the small rectangle in the corner)
   * The resizing always applies to the BASE image (layer 0), and the end size
   * should always be a multiple of 2^(num_layers) so the changes propagate
   * proportionally to all layers (example, changing 2 pixels in base image
   * should change 1 pixel in the layer 1)
   */
  ACTION_RESIZE,
  /*
   * When a buffer is added to the image, usually through a paste operation or
   * most of the tools like line, brush or text.
   * Indeed the brush tool for example works as "pasting" the result of the
   * brush whenever the user finishes dragging, the same for text, line and
   * bucket tools.
   */
  ACTION_BUFFER,
  /*
   * Adds a layer to an existing image.
   * The layer is built on top of the highest layer and has half the size of
   * the top layer.
   */
  ACTION_LAYER_PUSH,
  /*
   * Removes the top layer.
   */
  ACTION_LAYER_POP,
} HistCmdType;

// Command pattern for the image modifications.
// Used for the undo/redo history. Every change to the image pases through a
// command.
typedef struct HistCmd {
  HistCmdType actType;           /* action type */
  v2i offset;                    /* Offset in bottom coords */
  RectangleInt sel_rect;         /* Rectangle casted on tool for sel create*/
  v2i resize_delta;              /* Used in resizing action. */
  Image data_before[MAX_LAYERS]; /* Data before the tool at
                                    the target region. */
  Image data_after[MAX_LAYERS];  /* Data after applying the tool at the target
                                    region.*/
  Image paste_data[MAX_LAYERS];  /* Pasted buffers */
  Image resize_img_delta_x[MAX_LAYERS]; /* Delta image in resizing */
  Image resize_img_delta_y[MAX_LAYERS]; /* Delta image in resizing */
  bool multi_layer;
  int tool;             /* Tool used for the action. */
  int layer;            /* Layer of the modification. */
  struct HistCmd* next; /* Next element in the Linked list.*/
} HistCmd;

/*
 * Image history change manager.
 * This is the owner of the image buffers/textures.
 */
typedef struct {
  int layer;                   /* Active layer */
  tool_t tool;                 /* Active tool */
  Image buffer[MAX_LAYERS];    /* Working image buffer (real image)*/
  Image selbuffer[MAX_LAYERS]; /* Selection buffer (also stored as pyramid). */
  RenderTexture2D t_buffer[MAX_LAYERS];    /* Texture version of buffers, */
  RenderTexture2D t_selbuffer[MAX_LAYERS]; /* Texture version of selbuffers, */
  v2i seloff;           /* Offset of selection image  (bottom coord)*/
  HistCmd* hUndo;       /* First undo command (linked list) */
  HistCmd* hRedo;       /* First redo command (linked list) */
  bool dirty;           /* dirty flag for save */
  int maxUndoSize;      /* maximum size of undo history */
  int llsp[MAX_LAYERS]; /*log Spacing of each layer */
} Hist;

void hist_init(Hist* h);
void hist_destroy(Hist* h);
void hist_new_buffer(Hist* h, int bw, int bh);
void hist_act_change_image_size(Hist* h, int deltax, int deltay);
void hist_act_move_sel(Hist* h, int dx, int dy);
void hist_act_delete_sel(Hist* h);
void hist_act_fill_sel(Hist* h, Color fill_color);
void hist_act_flip_sel(Hist* h, HistCmdType act);
void hist_act_paste_image(Hist* h, v2i offset, int nl, Image* img);
void hist_act_buffer(Hist* h, Image img, v2i off);
void hist_act_commit_sel(Hist* h, RectangleInt tool_rect, bool multi_layer);
void hist_act_clone_sel(Hist* h);
void hist_act_layer_push(Hist* h);
void hist_act_layer_pop(Hist* h);
bool hist_get_has_selection(Hist* h);
void hist_set_buffer(Hist* h, int nl, Image* buffer);
void hist_set_tool(Hist* h, tool_t t);
tool_t hist_get_tool(Hist* h);
Image hist_get_active_buffer(Hist* h);
RectangleInt hist_get_sel_rect(Hist* h);
v2i hist_get_sel_offset(Hist* h);
bool hist_get_is_dirty(Hist* h);
void hist_set_not_dirty(Hist* h);
bool hist_get_can_undo(Hist* h);
bool hist_get_can_redo(Hist* h);
void hist_undo(Hist* h);
void hist_redo(Hist* h);
int hist_get_active_layer(Hist* h);
int hist_get_active_layer_llsp(Hist* h);
int hist_get_num_layers(Hist* h);
Image hist_export_sel(Hist* h);
void hist_set_layer(Hist* h, int layer);
bool hist_get_is_sel_multi(Hist* h);
Image hist_export_buf(Hist* h);
v2i hist_get_buf_size(Hist* h);
int hist_get_num_layers_sel(Hist* h);

#endif
