#include "hist.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "img.h"
#include "pyramid.h"

static Cmd* LoadCmd(ToolType t, CmdActionType act);
static void CmdUnload(Cmd* c);
static void CmdDo(Cmd* c, Hist* h);
static void CmdUndo(Cmd* c, Hist* h);
static RectangleInt AddOffsetToRect(RectangleInt r, Vector2Int v);

static void HistResetUndoHistory(Hist* h);
static void HistEmptyRedo(Hist* h);
static void HistEnsureUndoSize(Hist* h);
static void HistClearBuffer(Hist* h);

static void HistClearBufer(Hist* h) {
  if (h->buffer[0].width) {
    for (int i = 0; i < HIST_PYR_LVLS; i++) {
      UnloadImage(h->buffer[i]);
      h->buffer[i] = (Image){0};
    }
  }
  if (h->selbuffer[0].width) {
    for (int i = 0; i < HIST_PYR_LVLS; i++) {
      UnloadImage(h->selbuffer[i]);
      h->selbuffer[i] = (Image){0};
    }
  }
}

ToolType HistGetTool(Hist* h) { return h->tool; }

void HistSetTool(Hist* h, ToolType t) { h->tool = t; }

Image HistGetSelBuffer(Hist* h) { return h->selbuffer[0]; }

Vector2Int HistGetSelOffset(Hist* h) { return h->seloff; }

void HistLoad(Hist* h) {
  *h = (Hist){0};
  h->max_undo_hist_size = 50;
}

void HistUnload(Hist* h) { HistClearBufer(h); }

bool HistGetHasSelection(Hist* h) { return h->selbuffer[0].width > 0; }

void HistUndo(Hist* h) {
  if (!h->undo_hist) {
    return;
  }
  Cmd* c = h->undo_hist;
  CmdUndo(c, h);
  h->undo_hist = c->next;
  c->next = h->redo_hist;
  h->redo_hist = c;
}

void HistRedo(Hist* h) {
  if (!h->redo_hist) {
    return;
  }
  Cmd* c = h->redo_hist;
  CmdDo(c, h);
  h->redo_hist = c->next;
  c->next = h->undo_hist;
  h->undo_hist = c;
}

Image HistGetBuffer(Hist* h) { return h->buffer[0]; }

static RectangleInt AddOffsetToRect(RectangleInt r, Vector2Int v) {
  return (RectangleInt){
      .x = v.x,
      .y = v.y,
      .width = r.width,
      .height = r.height,
  };
}

void CmdUndo(Cmd* c, Hist* h) {
  h->dirty = true;
  if (c->next) {
    h->tool = c->next->tool;
  }
  switch (c->action_type) {
    case ACTION_BUFF: {
      RectangleInt r = {
          .x = 0,
          .y = 0,
          .width = c->data_after.width,
          .height = c->data_after.height,
      };
      CopyImage(c->data_before, r, &h->buffer[0], c->offset);
      PyramidUpdateRect2(HIST_PYR_LVLS, h->buffer,
                         AddOffsetToRect(r, c->offset));
      break;
    }
    case ACTION_SEL_CREATE: {
      if (c->sel_rect.width > 0) {
        if (c->paste_data.width == 0) {
          // This scenario is undoing a manual selection.
          // There's no need to check for borders because it's already done
          // when you perform the selection.
          // The destination should be empty so a copy should suffice (no need
          // to combine)
          RectangleInt r = {.x = 0,
                            .y = 0,
                            .width = h->selbuffer[0].width,
                            .height = h->selbuffer[0].height};
          Vector2Int off = (Vector2Int){.x = c->sel_rect.x, .y = c->sel_rect.y};
          ImageCombine(h->selbuffer[0], r, &h->buffer[0], off);
          PyramidUpdateRect2(HIST_PYR_LVLS, h->buffer, AddOffsetToRect(r, off));
        }
        UnloadImage(h->selbuffer[0]);
        UnloadImage(h->selbuffer[1]);
        UnloadImage(h->selbuffer[2]);
        h->selbuffer[0] = (Image){0};
        h->seloff = (Vector2Int){0};
      }
      if (c->data_after.width > 0) {
        // This part here is undoing a "commit" of a selection.
        // The selection itself could be outside of the volume region, so it's
        // important to check that we are pasting at the correct place.
        // Mind that data_before might be cropped because of that.
        assert(h->selbuffer[0].width == 0);
        h->selbuffer[0] = CloneImage(c->data_after);
        h->selbuffer[1] = PyramidGenImage(h->selbuffer[0]);
        h->selbuffer[2] = PyramidGenImage(h->selbuffer[1]);
        h->seloff = c->offset;
        // The c->offset  is the offset of the selection, not of the
        // data_before buffer, so we need to fix that.
        RectangleInt selrect = {
            .x = h->seloff.x,
            .y = h->seloff.y,
            .width = h->selbuffer[0].width,
            .height = h->selbuffer[0].height,
        };
        RectangleInt valid =
            GetCollisionRecInt(selrect, GetImageRect(h->buffer[0]));
        if (!IsRecIntEmpty(valid) && c->data_before.width > 0) {
          Vector2Int valid_offset = {.x = valid.x, .y = valid.y};
          // The valid region should have the same size as the data_before
          // buffer, since it was used for extraction of that buffer in the
          // first place...
          assert(c->data_before.width == valid.width);
          assert(c->data_before.height == valid.height);
          RectangleInt r = {
              .x = 0, .y = 0, .width = valid.width, .height = valid.height};
          CopyImage(c->data_before, r, &h->buffer[0], valid_offset);
          PyramidUpdateRect2(HIST_PYR_LVLS, h->buffer,
                             AddOffsetToRect(r, valid_offset));
        }
      }
      break;
    }
    case ACTION_SEL_MOVE: {
      h->seloff.x -= c->offset.x;
      h->seloff.y -= c->offset.y;
      break;
    }
    case ACTION_SEL_FLIP_H: {
      FlipImageHInplace(&h->selbuffer[0]);
      PyramidUpdateRect2(HIST_PYR_LVLS, h->selbuffer,
                         GetImageRect(h->selbuffer[0]));
      break;
    }
    case ACTION_SEL_FLIP_V: {
      FlipImageVInplace(&h->selbuffer[0]);
      PyramidUpdateRect2(HIST_PYR_LVLS, h->selbuffer,
                         GetImageRect(h->selbuffer[0]));
      break;
    }
    case ACTION_SEL_ROTATE: {
      Image tmp = h->selbuffer[0];
      h->selbuffer[0] = RotateImage(h->selbuffer[0], 1);
      UnloadImage(tmp);
      UnloadImage(h->selbuffer[1]);
      UnloadImage(h->selbuffer[2]);
      h->selbuffer[1] = PyramidGenImage(h->selbuffer[0]);
      h->selbuffer[2] = PyramidGenImage(h->selbuffer[1]);
      break;
    }
    case ACTION_RESIZE: {
      int deltax = c->resize_delta.x;
      int deltay = c->resize_delta.y;
      int new_w = h->buffer[0].width - deltax;
      int new_h = h->buffer[0].height - deltay;
      Image img1 = h->buffer[0];
      Image img2 = GenImageFilled(new_w, new_h, BLANK);
      Color* c0 = GetPixels(img1);
      Color* c1 = GetPixels(img2);
      // copies content of last image to new image.
      for (int y = 0; y < img1.height && y < img2.height; y++) {
        for (int x = 0; x < img1.width && x < img2.width; x++) {
          c1[y * img2.width + x] = c0[y * img1.width + x];
        }
      }
      h->buffer[0] = img2;
      UnloadImage(img1);
      UnloadImage(h->buffer[1]);
      UnloadImage(h->buffer[2]);
      if (c->resize_img_delta_x.width > 0) {
        int w = img2.width;
        int h = img2.height;
        int x0 = w + deltax;
        Color* dst = GetPixels(img2);
        Color* src = GetPixels(c->resize_img_delta_x);
        int ww = w - x0;
        assert(c->resize_img_delta_x.width == ww);
        for (int y = 0; y < h; y++) {
          for (int x = x0; x < w; x++) {
            dst[y * w + x] = src[y * ww + x - x0];
          }
        }
      }
      if (c->resize_img_delta_y.width > 0) {
        int w = img2.width;
        int h = img2.height;
        int y0 = h + deltay;
        Color* dst = GetPixels(img2);
        Color* src = GetPixels(c->resize_img_delta_y);
        assert(c->resize_img_delta_y.height == -deltay);
        for (int y = y0; y < h; y++) {
          for (int x = 0; x < w; x++) {
            dst[y * w + x] = src[(y - y0) * w + x];
          }
        }
      }
      h->buffer[1] = PyramidGenImage(h->buffer[0]);
      h->buffer[2] = PyramidGenImage(h->buffer[1]);
      break;
    };
  }
  HistEnsureUndoSize(h);
}

void CmdDo(Cmd* c, Hist* h) {
  h->dirty = true;
  h->tool = c->tool;
  switch (c->action_type) {
    case ACTION_BUFF: {
      RectangleInt r = {
          .x = 0,
          .y = 0,
          .width = c->data_after.width,
          .height = c->data_after.height,
      };
      ImageCombine(c->data_after, r, &h->buffer[0], c->offset);
      PyramidUpdateRect2(HIST_PYR_LVLS, h->buffer,
                         AddOffsetToRect(r, c->offset));
      break;
    }
    case ACTION_SEL_CREATE: {
      if (c->data_after.width > 0) {
        //  In this scenario, I have some data in the selection buffer, and as
        //  I'm creating a new one, I want to commit the existing contents to
        //  the main buffer.
        RectangleInt r = {
            .x = 0,
            .y = 0,
            .width = c->data_after.width,
            .height = c->data_after.height,
        };
        // Here, the sel rectangle might be off the image, so we fix it.
        Vector2Int off = c->offset;
        RectangleInt rdst = r;
        rdst.x = off.x;
        rdst.y = off.y;
        RectangleInt fixed_rdst =
            GetCollisionRecInt(rdst, GetImageRect(h->buffer[0]));
        // The scenario here is when the user drags the selection outside of the
        // image When we are deleting the selection, the data_before will be
        // empty.
        if (!IsRecIntEmpty(fixed_rdst) && c->data_before.width > 0) {
          // Adapts the small shift that might have had (ie new offset)
          r.x = fixed_rdst.x - off.x;
          r.y = fixed_rdst.y - off.y;
          // Uses the actual cropped shift
          off.x = fixed_rdst.x;
          off.y = fixed_rdst.y;
          // Uses the size of the cropped region
          r.width = fixed_rdst.width;
          r.height = fixed_rdst.height;
          ImageCombine(c->data_after, r, &h->buffer[0], off);
          PyramidUpdateRect2(HIST_PYR_LVLS, h->buffer, AddOffsetToRect(r, off));
        }
        UnloadImage(h->selbuffer[0]);
        UnloadImage(h->selbuffer[1]);
        UnloadImage(h->selbuffer[2]);
        h->seloff = (Vector2Int){0};
        h->selbuffer[0] = (Image){0};
      }
      h->seloff.x = c->sel_rect.x;
      h->seloff.y = c->sel_rect.y;
      if (c->sel_rect.width > 0) {
        // In this scenario, the user has selected a new selection region, so
        // the selection needs to be updated with the image's contents, and the
        // image itsef needs to be erased at that region.
        // Since the selection buffer is restricted by the image's size, we
        // don't need special treatment for region outside of the image (it
        // only appears when the selection is moved).
        assert(h->selbuffer[0].width == 0);
        if (c->paste_data.width == 0) {
          // Selection coming from mouse in the screen.
          h->selbuffer[0] =
              GenImageFilled(c->sel_rect.width, c->sel_rect.height, BLANK);
          CopyImage(h->buffer[0], c->sel_rect, &h->selbuffer[0],
                    (Vector2Int){0});
          FillImageRect(&h->buffer[0], c->sel_rect, BLANK);
          PyramidUpdateRect2(HIST_PYR_LVLS, h->buffer, c->sel_rect);
        } else {
          // Here we have a selection coming from a "Paste", or Ctr-V
          h->selbuffer[0] = CloneImage(c->paste_data);
        }
        h->selbuffer[1] = PyramidGenImage(h->selbuffer[0]);
        h->selbuffer[2] = PyramidGenImage(h->selbuffer[1]);
      }
      break;
    }
    case ACTION_SEL_MOVE: {
      h->seloff.x += c->offset.x;
      h->seloff.y += c->offset.y;
      break;
    }
    case ACTION_SEL_FLIP_H: {
      FlipImageHInplace(&h->selbuffer[0]);
      PyramidUpdateRect2(HIST_PYR_LVLS, h->selbuffer,
                         GetImageRect(h->selbuffer[0]));
      break;
    }
    case ACTION_SEL_FLIP_V: {
      FlipImageVInplace(&h->selbuffer[0]);
      PyramidUpdateRect2(HIST_PYR_LVLS, h->selbuffer,
                         GetImageRect(h->selbuffer[0]));
      break;
    }
    case ACTION_SEL_ROTATE: {
      Image tmp = h->selbuffer[0];
      h->selbuffer[0] = RotateImage(h->selbuffer[0], 0);
      UnloadImage(tmp);
      UnloadImage(h->selbuffer[1]);
      UnloadImage(h->selbuffer[2]);
      h->selbuffer[1] = PyramidGenImage(h->selbuffer[0]);
      h->selbuffer[2] = PyramidGenImage(h->selbuffer[1]);
      break;
    }
    case ACTION_RESIZE: {
      int new_w = h->buffer[0].width + c->resize_delta.x;
      int new_h = h->buffer[0].height + c->resize_delta.y;
      Image img1 = h->buffer[0];
      Image img2 = GenImageFilled(new_w, new_h, BLANK);
      Color* c0 = GetPixels(img1);
      Color* c1 = GetPixels(img2);
      // copies content of last image to new image.
      for (int y = 0; y < img1.height && y < img2.height; y++) {
        for (int x = 0; x < img1.width && x < img2.width; x++) {
          c1[y * img2.width + x] = c0[y * img1.width + x];
        }
      }
      h->buffer[0] = img2;
      UnloadImage(img1);
      UnloadImage(h->buffer[1]);
      UnloadImage(h->buffer[2]);
      h->buffer[1] = PyramidGenImage(h->buffer[0]);
      h->buffer[2] = PyramidGenImage(h->buffer[1]);
      break;
    };
  }
  HistEnsureUndoSize(h);
}

void HistActChangeImageSize(Hist* h, int deltax, int deltay) {
  if (deltax == 0 && deltay == 0) {
    return;
  }
  Cmd* c = LoadCmd(h->tool, ACTION_RESIZE);
  c->resize_delta = (Vector2Int){.x = deltax, .y = deltay};

  int w = h->buffer[0].width;
  int hh = h->buffer[0].height;
  if (deltax < 0) {
    RectangleInt xrect = {
        .x = w + deltax,
        .y = 0,
        .width = -deltax,
        .height = hh,
    };
    c->resize_img_delta_x = CropImage(h->buffer[0], xrect);
  }

  if (deltay < 0) {
    RectangleInt yrect = {
        .x = 0,
        .y = hh + deltay,
        .width = w,
        .height = -deltay,
    };
    c->resize_img_delta_y = CropImage(h->buffer[0], yrect);
  }
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistEmptyRedo(Hist* h) {
  while (h->redo_hist) {
    Cmd* c = h->redo_hist;
    h->redo_hist = c->next;
    CmdUnload(c);
  }
}

void HistActSelFlip(Hist* h, CmdActionType act) {
  Cmd* c = LoadCmd(TOOL_SEL, act);
  assert(h->selbuffer[0].width > 0);
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistResetUndoHistory(Hist* h) {
  HistEmptyRedo(h);
  while (h->undo_hist) {
    Cmd* c = h->undo_hist;
    h->undo_hist = c->next;
    CmdUnload(c);
  }
}

void HistActMoveSel(Hist* h, int dx, int dy) {
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_MOVE);
  c->offset = (Vector2Int){
      .x = dx,
      .y = dy,
  };
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistActDeleteSel(Hist* h) {
  if (h->selbuffer[0].width == 0) {
    return;
  }
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_CREATE);
  // makes the selection be empty after modif
  c->sel_rect = (RectangleInt){0};
  c->data_after = CloneImage(h->selbuffer[0]);
  c->offset = (Vector2Int){.x = h->seloff.x, .y = h->seloff.y};
  // I leave data_before as empty showing it has no impact on image
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistActSelFill(Hist* h, Color fill_color) {
  if (h->selbuffer[0].width == 0) {
    return;
  }
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_CREATE);
  c->data_after = CloneImage(h->selbuffer[0]);
  c->offset.x = h->seloff.x;
  c->offset.y = h->seloff.y;
  // I leave data_before as empty showing it has no impact on image

  c->next = h->undo_hist;
  c->paste_data =
      GenImageFilled(h->selbuffer[0].width, h->selbuffer[0].height, fill_color);
  c->sel_rect = (RectangleInt){
      .x = h->seloff.x,
      .y = h->seloff.y,
      .width = h->selbuffer[0].width,
      .height = h->selbuffer[0].height,
  };

  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistActBuffer(Hist* h, Image img, Vector2Int off) {
  if (img.width == 0) {
    return;
  }
  Cmd* c = LoadCmd(h->tool, ACTION_BUFF);
  c->offset = off;
  RectangleInt r = {
      .x = off.x,
      .y = off.y,
      .width = img.width,
      .height = img.height,
  };
  c->data_before = CropImage(h->buffer[0], r);
  c->data_after = img;
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistActCommitSel(Hist* h, RectangleInt tool_rect) {
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_CREATE);
  c->sel_rect = GetCollisionRecInt(tool_rect, GetImageRect(h->buffer[0]));
  // We consider rects of size 1 to be empty.
  bool is_empty = c->sel_rect.width * c->sel_rect.height <= 1;
  if (is_empty) {
    c->sel_rect = (RectangleInt){0};
  }
  if (h->selbuffer[0].width > 0) {
    c->data_after = CloneImage(h->selbuffer[0]);
    RectangleInt r = {
        .x = h->seloff.x,
        .y = h->seloff.y,
        .width = h->selbuffer[0].width,
        .height = h->selbuffer[0].height,
    };
    RectangleInt cropped_r = GetCollisionRecInt(r, GetImageRect(h->buffer[0]));
    if (cropped_r.width * cropped_r.height > 0) {
      c->data_before = CropImage(h->buffer[0], cropped_r);
    }
    c->offset = (Vector2Int){.x = h->seloff.x, .y = h->seloff.y};
  } else if (is_empty) {
    // Here we didnt have selection and the buffer is empty, so we cancel
    // the action.
    CmdUnload(c);
    return;
  }
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistActPasteImage(Hist* h, Vector2Int offset, Image img) {
  if (img.width == 0) {
    return;
  }
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_CREATE);

  if (h->selbuffer[0].width > 0) {
    c->data_after = CloneImage(h->selbuffer[0]);
    RectangleInt r = {
        .x = h->seloff.x,
        .y = h->seloff.y,
        .width = h->selbuffer[0].width,
        .height = h->selbuffer[0].height,
    };
    RectangleInt cropped_r = GetCollisionRecInt(r, GetImageRect(h->buffer[0]));
    if (cropped_r.width * cropped_r.height > 0) {
      c->data_before = CropImage(h->buffer[0], cropped_r);
    }
    c->offset = (Vector2Int){.x = h->seloff.x, .y = h->seloff.y};
  }

  c->sel_rect = (RectangleInt){
      .x = offset.x,
      .y = offset.y,
      .width = img.width,
      .height = img.height,
  };
  c->paste_data = CloneImage(img);
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistSetBuffer(Hist* h, Image buffer) {
  HistClearBufer(h);
  h->buffer[0] = buffer;
  // Here is the case where the loading has failed.
  if (h->buffer[0].width == 0) {
    int ww = 1024;
    int hh = 1024;
    h->buffer[0] = GenImageFilled(ww, hh, BLANK);
  }
  ImageRemoveBlacks(&h->buffer[0]);
  h->buffer[1] = PyramidGenImage(h->buffer[0]);
  h->buffer[2] = PyramidGenImage(h->buffer[1]);
  HistResetUndoHistory(h);
}

void HistNewBuffer(Hist* h, int bw, int bh) {
  HistClearBufer(h);
  int w = bw;
  int hh = bh;
  h->buffer[0] = GenImageFilled(w, hh, BLANK);
  h->buffer[1] = PyramidGenImage(h->buffer[0]);
  h->buffer[2] = PyramidGenImage(h->buffer[1]);
  HistResetUndoHistory(h);
  h->dirty = false;
}

bool HistGetIsDirty(Hist* h) { return h->dirty; }

void HistSetNotDirty(Hist* h) { h->dirty = false; }

bool HistGetCanUndo(Hist* h) { return h->undo_hist != NULL; }
bool HistGetCanRedo(Hist* h) { return h->redo_hist != NULL; };

void HistActCloneSel(Hist* h) {
  Image sel = HistGetSelBuffer(h);
  Vector2Int off = HistGetSelOffset(h);
  HistActPasteImage(h, off, sel);
}

void HistEnsureUndoSize(Hist* h) {
  int size = 0;
  int max_size = h->max_undo_hist_size;
  Cmd* c = h->undo_hist;
  while (c) {
    size += 1;
    c = c->next;
    if (size >= max_size) {
      break;
    }
  }
  if (c) {
    Cmd* t = c->next;
    c->next = NULL;
    while (t) {
      Cmd* q = t;
      t = t->next;
      CmdUnload(q);
    }
  }
}

Cmd* LoadCmd(ToolType t, CmdActionType act) {
  Cmd* c = calloc(1, sizeof(Cmd));
  c->tool = t;
  c->action_type = act;
  return c;
}

void CmdUnload(Cmd* c) {
  if (c->data_after.width > 0) {
    UnloadImage(c->data_after);
  }
  if (c->data_before.width > 0) {
    UnloadImage(c->data_before);
  }
  if (c->paste_data.width > 0) {
    UnloadImage(c->paste_data);
  }
  if (c->resize_img_delta_x.width > 0) {
    UnloadImage(c->resize_img_delta_x);
  }
  if (c->resize_img_delta_y.width > 0) {
    UnloadImage(c->resize_img_delta_y);
  }
  free(c);
}
