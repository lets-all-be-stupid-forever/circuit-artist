#include "hist.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "img.h"

/*
 * Creates and initialize a command.
 */
static HistCmd* hist_cmd_create(Hist* h, tool_t t, HistCmdType act) {
  HistCmd* c = calloc(1, sizeof(HistCmd));
  c->tool = t;
  c->layer = h->layer;
  c->actType = act;
  return c;
}

/*
 * Destructor for a command.
 * Will free any memory stored on it (CPU/GPU).
 */
static void hist_cmd_destroy(HistCmd* c) {
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (c->data_after[l].width > 0) {
      UnloadImage(c->data_after[l]);
    }
    if (c->data_before[l].width > 0) {
      UnloadImage(c->data_before[l]);
    }
    if (c->paste_data[l].width > 0) {
      UnloadImage(c->paste_data[l]);
    }
    if (c->resize_img_delta_x[l].width > 0) {
      UnloadImage(c->resize_img_delta_x[l]);
    }
    if (c->resize_img_delta_y[l].width > 0) {
      UnloadImage(c->resize_img_delta_y[l]);
    }
  }
  free(c);
}

/*
 * Ensures that the undo history is not taking too much memory.
 * The algorithm by nowjust take into account the number of commands instead of
 * actually memory. This needs rework.
 * TODO: Make it use a fixed-size cyclic buffer to avoid overflow.
 */
static void hist_ensure_undo_size(Hist* h) {
  int size = 0;
  int max_size = h->maxUndoSize;
  HistCmd* c = h->hUndo;
  while (c) {
    size += 1;
    c = c->next;
    if (size >= max_size) {
      break;
    }
  }
  if (c) {
    HistCmd* t = c->next;
    c->next = NULL;
    while (t) {
      HistCmd* q = t;
      t = t->next;
      hist_cmd_destroy(q);
    }
  }
}

/*
 * Applies the command FORWARD action.
 * Will modify the state of history.
 */
static void hist_cmd_do(HistCmd* c, Hist* h) {
  h->dirty = true;
  h->tool = c->tool;
  switch (c->actType) {
    case ACTION_BUFFER: {
      h->layer = c->layer;
      int l = c->layer;
      RectangleInt r = {
          .x = 0,
          .y = 0,
          .width = c->data_after[l].width,
          .height = c->data_after[l].height,
      };

      int ll = h->llsp[l];
      v2i off = {
          c->offset.x >> ll,
          c->offset.y >> ll,
      };
      image_combine(c->data_after[l], r, &h->buffer[l], off);
      RenderTexture2D tmp = clone_texture_from_image(c->data_after[l]);
      texture_combine(tmp, r, &h->t_buffer[l], off);
      UnloadRenderTexture(tmp);
      break;
    }
    case ACTION_SEL_CREATE: {
      /* Handles previous selection */
      for (int l = 0; l < MAX_LAYERS; l++) {
        if (c->data_after[l].width > 0) {
          //  In this scenario, I have some data in the selection buffer, and as
          //  I'm creating a new one, I want to commit the existing contents to
          //  the main buffer.
          RectangleInt r = {
              .x = 0,
              .y = 0,
              .width = c->data_after[l].width,
              .height = c->data_after[l].height,
          };
          int ll = h->llsp[l];
          assert(c->offset.x % (1 << ll) == 0);
          assert(c->offset.y % (1 << ll) == 0);
          // Here, the sel rectangle might be off the image, so we fix it.
          Vector2Int off = {
              c->offset.x >> ll,
              c->offset.y >> ll,
          };
          RectangleInt rdst = r;
          rdst.x = off.x;
          rdst.y = off.y;
          RectangleInt fixed_rdst =
              rect_int_get_collision(rdst, get_image_rect(h->buffer[l]));
          // The scenario here is when the user drags the selection outside of
          // the image When we are deleting the selection, the data_before will
          // be empty.
          if (!rect_int_is_empty(fixed_rdst) && c->data_before[l].width > 0) {
            // Adapts the small shift that might have had (ie new offset)
            r.x = fixed_rdst.x - off.x;
            r.y = fixed_rdst.y - off.y;
            // Uses the actual cropped shift
            off.x = fixed_rdst.x;
            off.y = fixed_rdst.y;
            // Uses the size of the cropped region
            r.width = fixed_rdst.width;
            r.height = fixed_rdst.height;
            image_combine(c->data_after[l], r, &h->buffer[l], off);
            RenderTexture2D t_data_after =
                clone_texture_from_image(c->data_after[l]);
            texture_combine(t_data_after, r, &h->t_buffer[l], off);
            UnloadRenderTexture(t_data_after);
          }
          UnloadRenderTexture(h->t_selbuffer[l]);
          UnloadImage(h->selbuffer[l]);
          h->seloff = (Vector2Int){0};
          h->selbuffer[l] = (Image){0};
          h->t_selbuffer[l] = (RenderTexture){0};
        }
      }
      /* Handles next selection */
      h->seloff.x = c->sel_rect.x;
      h->seloff.y = c->sel_rect.y;
      if (c->sel_rect.width > 0) {
        /* In this scenario, the user has selected a new selection region, so
         * the selection needs to be updated with the image's contents, and the
         * image itsef needs to be erased at that region.
         * Since the selection buffer is restricted by the image's size, we
         * don't need special treatment for region outside of the image (it
         * only appears when the selection is moved).
         *
         * Only goes up to the active layer during selection.
         */
        for (int l = 0; l <= c->layer; l++) {
          /* skips empty layers*/
          if (h->buffer[l].width == 0) continue;
          assert(h->selbuffer[l].width == 0);
          if (c->paste_data[l].width == 0) {
            /* Selection coming from mouse in the screen. */
            if (c->multi_layer || c->layer == l) {
              int ll = h->llsp[l];
              assert(c->sel_rect.width % (1 << ll) == 0);
              assert(c->sel_rect.height % (1 << ll) == 0);
              RectangleInt ri = {
                  .x = c->sel_rect.x >> ll,
                  .y = c->sel_rect.y >> ll,
                  .width = c->sel_rect.width >> ll,
                  .height = c->sel_rect.height >> ll,
              };
              /* updates selection */
              h->selbuffer[l] = gen_image_filled(ri.width, ri.height, BLANK);
              copy_image(h->buffer[l], ri, &h->selbuffer[l], (Vector2Int){0});
              h->t_selbuffer[l] = crop_texture(h->t_buffer[l], ri);
              fill_image_rect(&h->buffer[l], ri, BLANK);
              fill_texture_rect(&h->t_buffer[l], ri, BLACK);
            }
          } else {
            // Here we have a selection coming from a "Paste", or Ctr-V
            h->selbuffer[l] = clone_image(c->paste_data[l]);
            h->t_selbuffer[l] = clone_texture_from_image(c->paste_data[l]);
          }
        }
      }
      h->layer = c->layer;
      break;
    }
    case ACTION_SEL_MOVE: {
      h->seloff.x += c->offset.x;
      h->seloff.y += c->offset.y;
      break;
    }
    case ACTION_SEL_FLIP_H: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        flip_image_h_inplace(&h->selbuffer[l]);
        flip_texture_h_inplace(&h->t_selbuffer[l]);
      }
      break;
    }
    case ACTION_SEL_FLIP_V: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        flip_image_v_inplace(&h->selbuffer[l]);
        flip_texture_v_inplace(&h->t_selbuffer[l]);
      }
      break;
    }
    case ACTION_SEL_ROTATE: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        Image tmp = h->selbuffer[l];
        h->selbuffer[l] = rotate_image(h->selbuffer[l], 0);
        RenderTexture2D t_tmp = h->t_selbuffer[l];
        h->t_selbuffer[l] = rotate_texture(h->t_selbuffer[l], 0);
        UnloadRenderTexture(t_tmp);
        UnloadImage(tmp);
      }
      break;
    }
    case ACTION_RESIZE: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        if (h->buffer[l].width == 0) {
          continue;
        }

        int ll = h->llsp[l];
        int new_w = h->buffer[l].width + (c->resize_delta.x >> ll);
        int new_h = h->buffer[l].height + (c->resize_delta.y >> ll);
        Image img1 = h->buffer[l];
        Image img2 = gen_image_filled(new_w, new_h, BLANK);
        Color* c0 = get_pixels(img1);
        Color* c1 = get_pixels(img2);
        // copies content of last image to new image.
        for (int y = 0; y < img1.height && y < img2.height; y++) {
          for (int x = 0; x < img1.width && x < img2.width; x++) {
            c1[y * img2.width + x] = c0[y * img1.width + x];
          }
        }
        h->buffer[l] = img2;
        UnloadImage(img1);
        RenderTexture2D tex1 = h->t_buffer[l];
        RenderTexture2D tex2 = LoadRenderTexture(new_w, new_h);
        Vector2Int offset = {0, 0};
        RectangleInt source = {
            0.f,
            0.f,
            tex1.texture.width,
            tex1.texture.height,
        };
        copy_texture(tex1, source, &tex2, offset);
        h->t_buffer[l] = tex2;
        UnloadRenderTexture(tex1);
      }
      break;
    };
    case ACTION_LAYER_PUSH: {
      int top = -1;
      for (int l = 0; l < MAX_LAYERS; l++) {
        if (h->buffer[l].width > 0) {
          top = l;
        }
      }
      int l = top + 1;
      int ll = h->llsp[l];
      int w0 = h->buffer[0].width;
      int h0 = h->buffer[0].height;
      assert(!hist_get_has_selection(h));
      assert(w0 % (1 << ll) == 0);
      assert(h0 % (1 << ll) == 0);
      int w1 = w0 >> ll;
      int h1 = h0 >> ll;
      printf("buffer %d created with size %d %d\n", l, w1, h1);
      h->buffer[l] = gen_image_filled(w1, h1, BLANK);
      h->t_buffer[l] = clone_texture_from_image(h->buffer[l]);
      h->dirty = true;
      h->layer = l;
      break;
    }
    case ACTION_LAYER_POP: {
      int nl = hist_get_num_layers(h);
      assert(nl > 1);
      UnloadImage(h->buffer[nl - 1]);
      UnloadRenderTexture(h->t_buffer[nl - 1]);
      h->buffer[nl - 1] = (Image){0};
      h->t_buffer[nl - 1] = (RenderTexture2D){0};
      assert(!hist_get_has_selection(h));
      if (c->layer == nl - 1) {
        h->layer = nl - 2;
      }
      break;
    }
  }
  hist_ensure_undo_size(h);
}

/*
 * Applies a command BACKWARD.
 * The command should be the top of the undo history.
 */
static void hist_cmd_undo(HistCmd* c, Hist* h) {
  h->dirty = true;
  if (c->next) {
    h->tool = c->next->tool;
  }
  switch (c->actType) {
    case ACTION_BUFFER: {
      h->layer = c->layer;
      int l = c->layer;
      RectangleInt r = {
          .x = 0,
          .y = 0,
          .width = c->data_after[l].width,
          .height = c->data_after[l].height,
      };
      int ll = h->llsp[l];
      v2i off = {
          c->offset.x >> ll,
          c->offset.y >> ll,
      };
      copy_image(c->data_before[l], r, &h->buffer[l], off);
      RenderTexture2D tmp = clone_texture_from_image(c->data_before[l]);
      copy_texture(tmp, r, &h->t_buffer[l], off);
      UnloadRenderTexture(tmp);
      break;
    }
    case ACTION_SEL_CREATE: {
      if (c->sel_rect.width > 0) {
        for (int l = 0; l < MAX_LAYERS; l++) {
          if (h->buffer[l].width == 0) continue;
          if (c->paste_data[l].width == 0) {
            // This scenario is undoing a manual selection.
            // There's no need to check for borders because it's already done
            // when you perform the selection.
            // The destination should be empty so a copy should suffice (no need
            // to combine)
            RectangleInt r = {.x = 0,
                              .y = 0,
                              .width = h->selbuffer[l].width,
                              .height = h->selbuffer[l].height};
            int ll = h->llsp[l];
            Vector2Int off = (Vector2Int){.x = c->sel_rect.x >> ll,
                                          .y = c->sel_rect.y >> ll};
            image_combine(h->selbuffer[l], r, &h->buffer[l], off);
            texture_combine(h->t_selbuffer[l], r, &h->t_buffer[l], off);
          }
          UnloadImage(h->selbuffer[l]);
          UnloadRenderTexture(h->t_selbuffer[l]);
          h->selbuffer[l] = (Image){0};
          h->t_selbuffer[l] = (RenderTexture){0};
          h->seloff = (Vector2Int){0};
        }
      }
      for (int l = 0; l < MAX_LAYERS; l++) {
        if (c->data_after[l].width > 0) {
          // This part here is undoing a "commit" of a selection.
          // The selection it_selbufferf could be outside of the volume region,
          // so it's important to check that we are pasting at the correct
          // place. Mind that data_before might be cropped because of that.
          assert(h->selbuffer[l].width == 0);
          h->selbuffer[l] = clone_image(c->data_after[l]);
          h->t_selbuffer[l] = clone_texture_from_image(c->data_after[l]);
          h->seloff = c->offset;
          // The c->offset  is the offset of the selection, not of the
          // data_before buffer, so we need to fix that.

          int ll = h->llsp[l];
          RectangleInt selrect = {
              .x = h->seloff.x >> ll,
              .y = h->seloff.y >> ll,
              .width = h->selbuffer[l].width,
              .height = h->selbuffer[l].height,
          };
          RectangleInt valid =
              rect_int_get_collision(selrect, get_image_rect(h->buffer[l]));
          if (!rect_int_is_empty(valid) && c->data_before[l].width > 0) {
            Vector2Int valid_offset = {.x = valid.x, .y = valid.y};
            // The valid region should have the same size as the data_before
            // buffer, since it was used for extraction of that buffer in the
            // first place...
            assert(c->data_before[l].width == valid.width);
            assert(c->data_before[l].height == valid.height);
            RectangleInt r = {
                .x = 0, .y = 0, .width = valid.width, .height = valid.height};
            copy_image(c->data_before[l], r, &h->buffer[l], valid_offset);
            RenderTexture2D t_data_before =
                clone_texture_from_image(c->data_before[l]);
            copy_texture(t_data_before, r, &h->t_buffer[l], valid_offset);
            UnloadRenderTexture(t_data_before);
          }
        }
      }
      h->layer = c->layer;
      break;
    }
    case ACTION_SEL_MOVE: {
      h->seloff.x -= c->offset.x;
      h->seloff.y -= c->offset.y;
      break;
    }
    case ACTION_SEL_FLIP_H: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        flip_image_h_inplace(&h->selbuffer[l]);
        flip_texture_h_inplace(&h->t_selbuffer[l]);
      }
      break;
    }
    case ACTION_SEL_FLIP_V: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        flip_image_v_inplace(&h->selbuffer[l]);
        flip_texture_v_inplace(&h->t_selbuffer[l]);
      }
      break;
    }
    case ACTION_SEL_ROTATE: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        Image tmp = h->selbuffer[l];
        RenderTexture2D t_tmp = h->t_selbuffer[l];
        h->selbuffer[l] = rotate_image(h->selbuffer[l], 1);
        h->t_selbuffer[l] = rotate_texture(h->t_selbuffer[l], 1);
        UnloadRenderTexture(t_tmp);
        UnloadImage(tmp);
      }
      break;
    }
    case ACTION_RESIZE: {
      for (int l = 0; l < MAX_LAYERS; l++) {
        if (h->buffer[l].width == 0) continue;

        int ll = h->llsp[l];
        int deltax = c->resize_delta.x >> ll;
        int deltay = c->resize_delta.y >> ll;
        int new_w = h->buffer[l].width - deltax;
        int new_h = h->buffer[l].height - deltay;
        Image img1 = h->buffer[l];
        Image img2 = gen_image_filled(new_w, new_h, BLANK);
        Color* c0 = get_pixels(img1);
        Color* c1 = get_pixels(img2);
        // copies content of last image to new image.
        for (int y = 0; y < img1.height && y < img2.height; y++) {
          for (int x = 0; x < img1.width && x < img2.width; x++) {
            c1[y * img2.width + x] = c0[y * img1.width + x];
          }
        }
        h->buffer[l] = img2;
        UnloadImage(img1);

        RenderTexture2D tex1 = h->t_buffer[l];
        RenderTexture2D tex2 = LoadRenderTexture(new_w, new_h);
        Vector2Int offset = {0, 0};
        RectangleInt source = {
            0.f,
            0.f,
            tex1.texture.width,
            tex1.texture.height,
        };
        copy_texture(tex1, source, &tex2, offset);
        h->t_buffer[l] = tex2;
        UnloadRenderTexture(tex1);
        if (c->resize_img_delta_x[l].width > 0) {
          int w = img2.width;
          int h = img2.height;
          int x0 = w + deltax;
          Color* dst = get_pixels(img2);
          Color* src = get_pixels(c->resize_img_delta_x[l]);
          int ww = w - x0;
          assert(c->resize_img_delta_x[l].width == ww);
          for (int y = 0; y < h; y++) {
            for (int x = x0; x < w; x++) {
              dst[y * w + x] = src[y * ww + x - x0];
            }
          }
          Vector2Int offset = {x0, 0};
          RectangleInt source = {0, 0, c->resize_img_delta_x[l].width, h};
          RenderTexture2D t_resize_img_delta_x =
              clone_texture_from_image(c->resize_img_delta_x[l]);
          copy_texture(t_resize_img_delta_x, source, &tex2, offset);
          UnloadRenderTexture(t_resize_img_delta_x);
        }
        if (c->resize_img_delta_y[l].width > 0) {
          int w = img2.width;
          int h = img2.height;
          int y0 = h + deltay;
          Color* dst = get_pixels(img2);
          Color* src = get_pixels(c->resize_img_delta_y[l]);
          assert(c->resize_img_delta_y[l].height == -deltay);
          for (int y = y0; y < h; y++) {
            for (int x = 0; x < w; x++) {
              dst[y * w + x] = src[(y - y0) * w + x];
            }
          }
          Vector2Int offset = {0, y0};
          RectangleInt source = {0, 0, w, c->resize_img_delta_y[l].height};
          RenderTexture2D t_resize_img_delta_y =
              clone_texture_from_image(c->resize_img_delta_y[l]);
          copy_texture(t_resize_img_delta_y, source, &tex2, offset);
          UnloadRenderTexture(t_resize_img_delta_y);
        }
      }
      break;
    };
    case ACTION_LAYER_PUSH: {
      assert(!hist_get_has_selection(h));
      int nl = hist_get_num_layers(h);
      assert(nl > 1);
      UnloadImage(h->buffer[nl - 1]);
      UnloadRenderTexture(h->t_buffer[nl - 1]);
      h->buffer[nl - 1] = (Image){0};
      h->t_buffer[nl - 1] = (RenderTexture2D){0};
      h->layer = c->layer;
      printf("buffer removed.\n");
      break;
    }
    case ACTION_LAYER_POP: {
      int nl = hist_get_num_layers(h);
      h->buffer[nl] = clone_image(c->data_before[nl]);
      h->t_buffer[nl] = clone_texture_from_image(c->data_before[nl]);
      assert(!hist_get_has_selection(h));
      h->layer = c->layer;
      break;
    }
  }
  hist_ensure_undo_size(h);
}

/*
 * Clears REDO history.
 * Usually when you create a new action, you want first to remove the previous
 * existing REDO history.
 */
static void hist_empty_redo(Hist* h) {
  while (h->hRedo) {
    HistCmd* c = h->hRedo;
    h->hRedo = c->next;
    hist_cmd_destroy(c);
  }
}

/*
 * Clears both UNDO and REDO histories.
 */
static void hist_reset_undo_history(Hist* h) {
  hist_empty_redo(h);
  while (h->hUndo) {
    HistCmd* c = h->hUndo;
    h->hUndo = c->next;
    hist_cmd_destroy(c);
  }
}

/*
 * Sets the (x,y) position of a rectangle.
 */
static RectangleInt add_offset_to_rec(RectangleInt r, Vector2Int v) {
  return (RectangleInt){
      .x = v.x,
      .y = v.y,
      .width = r.width,
      .height = r.height,
  };
}

static void hist_clear_buffer(Hist* h) {
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->buffer[l].width) {
      UnloadImage(h->buffer[l]);
      h->buffer[l] = (Image){0};
      UnloadRenderTexture(h->t_buffer[l]);
      h->t_buffer[l] = (RenderTexture2D){0};
    }
    if (h->selbuffer[l].width) {
      UnloadImage(h->selbuffer[l]);
      h->selbuffer[l] = (Image){0};
      UnloadRenderTexture(h->t_selbuffer[l]);
      h->t_selbuffer[l] = (RenderTexture2D){0};
    }
  }
}

// Returns the active tool.
tool_t hist_get_tool(Hist* h) { return h->tool; }

// Changes the active tool.
// For example, when user changes a tool in the UI.
// Mind though that this is a low-level function, it won't do things like
// commiting selection when the hte user changes from marquee to brush. This
// side is handled elsewhere.
void hist_set_tool(Hist* h, tool_t t) { h->tool = t; }

// Returns the active image selection buffer as reference (marquee
// selection).. If there's no selection, an empty image is returned. (as in
// width=0, height=0, data=NULL)
// Image hist_get_sel_buffer(Hist* h) { return h->selbuffer[]; }

Vector2Int hist_get_sel_offset(Hist* h) { return h->seloff; }

// Initializes History change buffers.
void hist_init(Hist* h) {
  *h = (Hist){0};
  h->maxUndoSize = 50;
  h->llsp[0] = 0;
#if 0 /* logarithm layers */
  h->llsp[1] = 1;
  h->llsp[2] = 2;
  h->llsp[3] = 3;
#else /* constant layers */
  h->llsp[1] = 0;
  h->llsp[2] = 0;
  // h->llsp[3] = 0;
#endif
}

void hist_destroy(Hist* h) { hist_clear_buffer(h); }

// Returns true if there's a region selected.
bool hist_get_has_selection(Hist* h) {
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->selbuffer[l].width > 0) {
      return true;
    }
  }
  return false;
}

void hist_undo(Hist* h) {
  if (!h->hUndo) {
    return;
  }
  HistCmd* c = h->hUndo;
  hist_cmd_undo(c, h);
  h->hUndo = c->next;
  c->next = h->hRedo;
  h->hRedo = c;
}

void hist_redo(Hist* h) {
  if (!h->hRedo) {
    return;
  }
  HistCmd* c = h->hRedo;
  hist_cmd_do(c, h);
  h->hRedo = c->next;
  c->next = h->hUndo;
  h->hUndo = c;
}

// Dispatchs an action to resize the image.
void hist_act_change_image_size(Hist* h, int deltax, int deltay) {
  if (deltax == 0 && deltay == 0) {
    return;
  }
  HistCmd* c = hist_cmd_create(h, h->tool, ACTION_RESIZE);
  c->resize_delta = (Vector2Int){.x = deltax, .y = deltay};

  for (int l = 0; l < MAX_LAYERS; l++) {
    int w = h->buffer[l].width;
    int hh = h->buffer[l].height;
    if (w == 0 || hh == 0) continue;

    int ll = h->llsp[l];
    if (deltax < 0) {
      int dx = (-deltax) >> ll;
      RectangleInt xrect = {
          .x = (w - dx),
          .y = 0,
          .width = dx,
          .height = hh,
      };
      c->resize_img_delta_x[l] = crop_image(h->buffer[l], xrect);
    }

    if (deltay < 0) {
      int dy = (-deltay) >> ll;
      RectangleInt yrect = {
          .x = 0,
          .y = hh - dy,
          .width = w,
          .height = dy,
      };
      c->resize_img_delta_y[l] = crop_image(h->buffer[l], yrect);
    }
  }

  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

// Dispatches an action to flip a selection.
// act should be  one of: ACTION_SEL_FLIP_H, ACTION_SEL_FLIP_V or
// ACTION_SEL_ROTATE.
void hist_act_flip_sel(Hist* h, HistCmdType act) {
  HistCmd* c = hist_cmd_create(h, TOOL_SEL, act);
  assert(h->selbuffer[h->layer].width > 0);
  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

// Dispatches an action to move a marquee selection.
void hist_act_move_sel(Hist* h, int dx, int dy) {
  HistCmd* c = hist_cmd_create(h, TOOL_SEL, ACTION_SEL_MOVE);
  c->offset = (Vector2Int){
      .x = dx,
      .y = dy,
  };
  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

// Dispatches an action to delete the current marquee selection.
void hist_act_delete_sel(Hist* h) {
  if (!hist_get_has_selection(h)) {
    return;
  }
  HistCmd* c = hist_cmd_create(h, TOOL_SEL, ACTION_SEL_CREATE);
  // makes the selection be empty after modif
  c->sel_rect = (RectangleInt){0};
  c->offset = (Vector2Int){.x = h->seloff.x, .y = h->seloff.y};
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->selbuffer[l].width > 0) {
      c->data_after[l] = clone_image(h->selbuffer[l]);
    }
  }
  // I leave data_before as empty showing it has no impact on image
  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

// Dispatches an action to fill the selection (marquee's fill buttton).
void hist_act_fill_sel(Hist* h, Color fill_color) {
  if (!hist_get_has_selection(h)) {
    return;
  }
  HistCmd* c = hist_cmd_create(h, TOOL_SEL, ACTION_SEL_CREATE);
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->selbuffer[l].width > 0) {
      c->data_after[l] = clone_image(h->selbuffer[l]);
    }
  }
  c->offset.x = h->seloff.x;
  c->offset.y = h->seloff.y;
  // I leave data_before as empty showing it has no impact on image

  c->next = h->hUndo;
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->selbuffer[l].width > 0) {
      c->paste_data[l] = gen_image_filled(h->selbuffer[l].width,
                                          h->selbuffer[l].height, fill_color);
      int ll = h->llsp[l];
      c->sel_rect = (RectangleInt){
          .x = h->seloff.x,
          .y = h->seloff.y,
          .width = h->selbuffer[l].width << ll,
          .height = h->selbuffer[l].height << ll,
      };
    }
  }

  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

/*
 * Dispatches an action to apply a buffer.
 * Often used with tools like line/brush/text/bucket.
 * It will take ownership of the image's data.
 */
void hist_act_buffer(Hist* h, Image img, Vector2Int off) {
  if (img.width == 0) {
    return;
  }
  HistCmd* c = hist_cmd_create(h, h->tool, ACTION_BUFFER);
  c->offset = off;
  int ll = h->llsp[h->layer];
  assert(off.x % (1 << ll) == 0);
  assert(off.y % (1 << ll) == 0);

  RectangleInt r = {
      .x = off.x >> ll,
      .y = off.y >> ll,
      .width = img.width,
      .height = img.height,
  };
  c->data_before[c->layer] = crop_image(h->buffer[c->layer], r);
  c->data_after[c->layer] = img;
  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

// Dispatches an action to de-select a selection. (clicking outside, pressing
// ESC, changing tool etc)
void hist_act_commit_sel(Hist* h, RectangleInt tool_rect, bool multi_layer) {
  HistCmd* c = hist_cmd_create(h, TOOL_SEL, ACTION_SEL_CREATE);
  c->sel_rect = rect_int_get_collision(tool_rect, get_image_rect(h->buffer[0]));
  // We consider rects of size 1 to be empty.
  bool is_empty = c->sel_rect.width * c->sel_rect.height <= 1;
  if (is_empty) {
    c->sel_rect = (RectangleInt){0};
  }
  c->layer = h->layer;
  c->multi_layer = multi_layer;
  bool had_sel = false;
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->selbuffer[l].width > 0) {
      had_sel = true;
      c->data_after[l] = clone_image(h->selbuffer[l]);
      int ll = h->llsp[l];
      RectangleInt r = {
          .x = h->seloff.x >> ll,
          .y = h->seloff.y >> ll,
          .width = h->selbuffer[l].width,
          .height = h->selbuffer[l].height,
      };
      RectangleInt cropped_r =
          rect_int_get_collision(r, get_image_rect(h->buffer[l]));
      if (cropped_r.width * cropped_r.height > 0) {
        c->data_before[l] = crop_image(h->buffer[l], cropped_r);
      }
      c->offset = (Vector2Int){.x = h->seloff.x, .y = h->seloff.y};
    }
  }
  if (!had_sel && is_empty) {
    // Here we didnt have selection and the buffer is empty, so we cancel
    // the action.
    hist_cmd_destroy(c);
    return;
  }
  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

/*
 * Dispatch an action to paste a new image at a given place (C-V).
 * Takes ownership of the input image(s).
 */
void hist_act_paste_image(Hist* h, Vector2Int offset, int nl, Image* img) {
  int h_nl = hist_get_num_layers(h);
  /* Need to handle max layers outside the paste: need to crop/throw away extra
   * stuff and/or send a warning message */
  assert(nl <= h_nl);
  HistCmd* c = hist_cmd_create(h, TOOL_SEL, ACTION_SEL_CREATE);

  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->selbuffer[l].width > 0) {
      c->data_after[l] = clone_image(h->selbuffer[l]);
      int ll = h->llsp[l];
      RectangleInt r = {
          .x = h->seloff.x >> ll,
          .y = h->seloff.y >> ll,
          .width = h->selbuffer[l].width,
          .height = h->selbuffer[l].height,
      };
      RectangleInt cropped_r =
          rect_int_get_collision(r, get_image_rect(h->buffer[l]));
      if (cropped_r.width * cropped_r.height > 0) {
        c->data_before[l] = crop_image(h->buffer[l], cropped_r);
      }
      c->offset = (Vector2Int){.x = h->seloff.x, .y = h->seloff.y};
    }
  }

  printf("PASTE_NL=%d\n", nl);
  if (nl == 1) {
    c->layer = h->layer;
    int ll = h->llsp[h->layer];
    c->sel_rect = (RectangleInt){
        .x = (offset.x >> ll) << ll,
        .y = (offset.y >> ll) << ll,
        .width = img[0].width << ll,
        .height = img[0].height << ll,
    };
    // Pastes on current layer
    c->paste_data[h->layer] = img[0];
  } else {
    c->layer = nl - 1;
    for (int l = 0; l <= c->layer; l++) {
      int ll = h->llsp[l];
      /* Offset and size is set by the top layer to ensure offset consistency
       */
      if (l == c->layer) {
        c->sel_rect = (RectangleInt){
            .x = (offset.x >> ll) << ll,
            .y = (offset.y >> ll) << ll,
            .width = img[l].width << ll,
            .height = img[l].height << ll,
        };
      }
      /* Pastes on current layer */
      c->paste_data[l] = img[l];
    }
  }
  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

/*
 * Sets the working buffer of the history.
 * Often called at the very start when we just loaded an image from the disk.
 * Takes OWNERSHIP of the input buffer.
 */
void hist_set_buffer(Hist* h, int nl, Image* buffer) {
  assert(buffer[0].width > 0);
  assert(nl > 0);

  hist_clear_buffer(h);
  if (nl > MAX_LAYERS) {
    printf("Can't import image: too many layers (%d)\n.", nl);
    abort();
  }
  for (int l = 0; l < nl; l++) {
    int ll = h->llsp[l];
    assert(buffer[l].width == (buffer[0].width >> ll));
    assert(buffer[l].height == (buffer[0].height >> ll));
    h->buffer[l] = buffer[l];
    h->t_buffer[l] = clone_texture_from_image(h->buffer[l]);
  }
  h->layer = 0;
  h->dirty = false;
  hist_reset_undo_history(h);
}

// Resets history and creates an empty image. (New button in the UI)
void hist_new_buffer(Hist* h, int bw, int bh) {
  Image img = gen_image_filled(bw, bh, BLANK);
  hist_set_buffer(h, 1, &img);
}

// Getter to whether there was any change in the image.
// Often used when we want to check whether we need to save before opening a
// new image or before exiting the game.
bool hist_get_is_dirty(Hist* h) { return h->dirty; }

// Resets the dirty flag.
// Used after we save an image for example.
void hist_set_not_dirty(Hist* h) { h->dirty = false; }

// Returns whether it's possible to perform and undo.
bool hist_get_can_undo(Hist* h) { return h->hUndo != NULL; }

// Returns whether it's possible to perform and redo.
bool hist_get_can_redo(Hist* h) { return h->hRedo != NULL; };

// Dispatches an action for duplicating current selection at the beginning of
// a drag. Use case is when we drag a selection with the left CTRL key
// pressed.
void hist_act_clone_sel(Hist* h) {
  Vector2Int off = hist_get_sel_offset(h);
  Image tmp[20] = {0};
  int nl;
  int multi = hist_get_is_sel_multi(h);
  if (multi) {
    nl = h->layer + 1;
    for (int l = 0; l < nl; l++) {
      tmp[l] = clone_image(h->selbuffer[l]);
    }
  } else {
    nl = 1;
    tmp[0] = clone_image(h->selbuffer[h->layer]);
  }
  hist_act_paste_image(h, off, nl, tmp);
}

void hist_act_layer_push(Hist* h) {
  HistCmd* c = hist_cmd_create(h, h->tool, ACTION_LAYER_PUSH);
  c->layer = h->layer;
  c->next = h->hUndo;
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

void hist_act_layer_pop(Hist* h) {
  HistCmd* c = hist_cmd_create(h, h->tool, ACTION_LAYER_POP);
  c->layer = h->layer;
  c->next = h->hUndo;
  int nl = hist_get_num_layers(h);
  assert(nl > 1);
  c->data_before[nl - 1] = clone_image(h->buffer[nl - 1]);
  h->hUndo = c;
  hist_empty_redo(h);
  hist_cmd_do(c, h);
}

int hist_get_active_layer(Hist* h) { return h->layer; }

int hist_get_num_layers_sel(Hist* h) {
  int m = 0;
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->selbuffer[l].width > 0) m = l + 1;
  }
  return m;
}

int hist_get_num_layers(Hist* h) {
  int m = 0;
  for (int l = 0; l < MAX_LAYERS; l++) {
    if (h->buffer[l].width > 0) m = l + 1;
  }
  return m;
}

Image hist_get_active_buffer(Hist* h) { return h->buffer[h->layer]; }

RectangleInt hist_get_sel_rect(Hist* h) {
  for (int l = 0; l < MAX_LAYERS; l++) {
    int bw = h->selbuffer[l].width;
    int bh = h->selbuffer[l].height;
    if (bw > 0) {
      int ll = h->llsp[l];
      return (RectangleInt){
          .x = h->seloff.x,
          .y = h->seloff.y,
          .width = bw << ll,
          .height = bh << ll,
      };
    }
  }
  return (RectangleInt){0};
}

Image hist_export_sel(Hist* h) {
  if (!hist_get_has_selection(h)) {
    return (Image){0};
  }
  if (hist_get_is_sel_multi(h)) {
    /* multi-layer selection*/
    int nl = h->layer + 1;
    return image_encode_layers(nl, h->selbuffer);
  } else {
    /* single-layer selection*/
    return image_encode_layers(1, &h->selbuffer[h->layer]);
  }
}

void hist_set_layer(Hist* h, int layer) { h->layer = layer; }

int hist_get_active_layer_llsp(Hist* h) { return h->llsp[h->layer]; }

bool hist_get_is_sel_multi(Hist* h) {
  return h->selbuffer[0].width > 0 && h->selbuffer[1].width > 0;
}

Image hist_export_buf(Hist* h) {
  return image_encode_layers(hist_get_num_layers(h), h->buffer);
}

v2i hist_get_buf_size(Hist* h) {
  return (v2i){
      h->buffer[0].width,
      h->buffer[0].height,
  };
}
