#include "hist.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "img.h"

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
  if (h->buffer.width) {
    UnloadImage(h->buffer);
    h->buffer = (Image){0};
    UnloadRenderTexture(h->t_buffer);
  }
  if (h->selbuffer.width) {
    for (int i = 0; i < HIST_PYR_LVLS; i++) {
      UnloadImage(h->selbuffer);
      h->selbuffer = (Image){0};
    }
    UnloadRenderTexture(h->t_selbuffer);
  }
}

ToolType HistGetTool(Hist* h) { return h->tool; }

void HistSetTool(Hist* h, ToolType t) { h->tool = t; }

Image HistGetSelBuffer(Hist* h) { return h->selbuffer; }

Vector2Int HistGetSelOffset(Hist* h) { return h->seloff; }

void HistLoad(Hist* h) {
  *h = (Hist){0};
  h->max_undo_hist_size = 50;
}

void HistUnload(Hist* h) { HistClearBufer(h); }

bool HistGetHasSelection(Hist* h) { return h->selbuffer.width > 0; }

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

Image HistGetBuffer(Hist* h) { return h->buffer; }

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
      CopyImage(c->data_before, r, &h->buffer, c->offset);
      CopyTexture(c->t_data_before, r, &h->t_buffer, c->offset);
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
                            .width = h->selbuffer.width,
                            .height = h->selbuffer.height};
          Vector2Int off = (Vector2Int){.x = c->sel_rect.x, .y = c->sel_rect.y};
          ImageCombine(h->selbuffer, r, &h->buffer, off);
          TextureCombine(h->t_selbuffer, r, &h->t_buffer, off);
        }
        UnloadImage(h->selbuffer);
        UnloadRenderTexture(h->t_selbuffer);
        h->selbuffer = (Image){0};
        h->t_selbuffer = (RenderTexture){0};
        h->seloff = (Vector2Int){0};
      }
      if (c->data_after.width > 0) {
        // This part here is undoing a "commit" of a selection.
        // The selection itself could be outside of the volume region, so it's
        // important to check that we are pasting at the correct place.
        // Mind that data_before might be cropped because of that.
        assert(h->selbuffer.width == 0);
        h->selbuffer = CloneImage(c->data_after);
        h->t_selbuffer = CloneTexture(c->t_data_after);
        h->seloff = c->offset;
        // The c->offset  is the offset of the selection, not of the
        // data_before buffer, so we need to fix that.
        RectangleInt selrect = {
            .x = h->seloff.x,
            .y = h->seloff.y,
            .width = h->selbuffer.width,
            .height = h->selbuffer.height,
        };
        RectangleInt valid =
            GetCollisionRecInt(selrect, GetImageRect(h->buffer));
        if (!IsRecIntEmpty(valid) && c->data_before.width > 0) {
          Vector2Int valid_offset = {.x = valid.x, .y = valid.y};
          // The valid region should have the same size as the data_before
          // buffer, since it was used for extraction of that buffer in the
          // first place...
          assert(c->data_before.width == valid.width);
          assert(c->data_before.height == valid.height);
          RectangleInt r = {
              .x = 0, .y = 0, .width = valid.width, .height = valid.height};
          CopyImage(c->data_before, r, &h->buffer, valid_offset);
          CopyTexture(c->t_data_before, r, &h->t_buffer, valid_offset);
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
      FlipImageHInplace(&h->selbuffer);
      FlipTextureHInplace(&h->t_selbuffer);
      break;
    }
    case ACTION_SEL_FLIP_V: {
      FlipImageVInplace(&h->selbuffer);
      FlipTextureVInplace(&h->t_selbuffer);
      break;
    }
    case ACTION_SEL_ROTATE: {
      Image tmp = h->selbuffer;
      RenderTexture2D t_tmp = h->t_selbuffer;
      h->selbuffer = RotateImage(h->selbuffer, 1);
      h->t_selbuffer = RotateTexture(h->t_selbuffer, 1);
      UnloadRenderTexture(t_tmp);
      UnloadImage(tmp);
      break;
    }
    case ACTION_RESIZE: {
      int deltax = c->resize_delta.x;
      int deltay = c->resize_delta.y;
      int new_w = h->buffer.width - deltax;
      int new_h = h->buffer.height - deltay;
      Image img1 = h->buffer;
      Image img2 = GenImageFilled(new_w, new_h, BLANK);
      Color* c0 = GetPixels(img1);
      Color* c1 = GetPixels(img2);
      // copies content of last image to new image.
      for (int y = 0; y < img1.height && y < img2.height; y++) {
        for (int x = 0; x < img1.width && x < img2.width; x++) {
          c1[y * img2.width + x] = c0[y * img1.width + x];
        }
      }
      h->buffer = img2;
      UnloadImage(img1);

      RenderTexture2D tex1 = h->t_buffer;
      RenderTexture2D tex2 = LoadRenderTexture(new_w, new_h);
      Vector2Int offset = {0, 0};
      RectangleInt source = {
          0.f,
          0.f,
          tex1.texture.width,
          tex1.texture.height,
      };
      CopyTexture(tex1, source, &tex2, offset);
      h->t_buffer = tex2;
      UnloadRenderTexture(tex1);
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
        Vector2Int offset = {x0, 0};
        RectangleInt source = {0, 0, c->resize_img_delta_x.width, h};
        CopyTexture(c->t_resize_img_delta_x, source, &tex2, offset);
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
        Vector2Int offset = {0, y0};
        RectangleInt source = {0, 0, w, c->resize_img_delta_y.height};
        CopyTexture(c->t_resize_img_delta_y, source, &tex2, offset);
      }
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
      ImageCombine(c->data_after, r, &h->buffer, c->offset);
      TextureCombine(c->t_data_after, r, &h->t_buffer, c->offset);
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
            GetCollisionRecInt(rdst, GetImageRect(h->buffer));
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
          ImageCombine(c->data_after, r, &h->buffer, off);
          TextureCombine(c->t_data_after, r, &h->t_buffer, off);
        }
        UnloadRenderTexture(h->t_selbuffer);
        UnloadImage(h->selbuffer);
        h->seloff = (Vector2Int){0};
        h->selbuffer = (Image){0};
        h->t_selbuffer = (RenderTexture){0};
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
        assert(h->selbuffer.width == 0);
        if (c->paste_data.width == 0) {
          // Selection coming from mouse in the screen.
          h->selbuffer =
              GenImageFilled(c->sel_rect.width, c->sel_rect.height, BLANK);
          CopyImage(h->buffer, c->sel_rect, &h->selbuffer, (Vector2Int){0});
          h->t_selbuffer = CropTexture(h->t_buffer, c->sel_rect);
          FillImageRect(&h->buffer, c->sel_rect, BLANK);
          FillTextureRect(&h->t_buffer, c->sel_rect, BLACK);
        } else {
          // Here we have a selection coming from a "Paste", or Ctr-V
          h->selbuffer = CloneImage(c->paste_data);
          h->t_selbuffer = CloneTexture(c->t_paste_data);
        }
      }
      break;
    }
    case ACTION_SEL_MOVE: {
      h->seloff.x += c->offset.x;
      h->seloff.y += c->offset.y;
      break;
    }
    case ACTION_SEL_FLIP_H: {
      FlipImageHInplace(&h->selbuffer);
      FlipTextureHInplace(&h->t_selbuffer);
      break;
    }
    case ACTION_SEL_FLIP_V: {
      FlipImageVInplace(&h->selbuffer);
      FlipTextureVInplace(&h->t_selbuffer);
      break;
    }
    case ACTION_SEL_ROTATE: {
      Image tmp = h->selbuffer;
      h->selbuffer = RotateImage(h->selbuffer, 0);
      RenderTexture2D t_tmp = h->t_selbuffer;
      h->t_selbuffer = RotateTexture(h->t_selbuffer, 0);
      UnloadRenderTexture(t_tmp);
      UnloadImage(tmp);
      break;
    }
    case ACTION_RESIZE: {
      int new_w = h->buffer.width + c->resize_delta.x;
      int new_h = h->buffer.height + c->resize_delta.y;
      Image img1 = h->buffer;
      Image img2 = GenImageFilled(new_w, new_h, BLANK);
      Color* c0 = GetPixels(img1);
      Color* c1 = GetPixels(img2);
      // copies content of last image to new image.
      for (int y = 0; y < img1.height && y < img2.height; y++) {
        for (int x = 0; x < img1.width && x < img2.width; x++) {
          c1[y * img2.width + x] = c0[y * img1.width + x];
        }
      }
      h->buffer = img2;
      UnloadImage(img1);
      RenderTexture2D tex1 = h->t_buffer;
      RenderTexture2D tex2 = LoadRenderTexture(new_w, new_h);
      Vector2Int offset = {0, 0};
      RectangleInt source = {
          0.f,
          0.f,
          tex1.texture.width,
          tex1.texture.height,
      };
      CopyTexture(tex1, source, &tex2, offset);
      h->t_buffer = tex2;
      UnloadRenderTexture(tex1);
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

  int w = h->buffer.width;
  int hh = h->buffer.height;
  if (deltax < 0) {
    RectangleInt xrect = {
        .x = w + deltax,
        .y = 0,
        .width = -deltax,
        .height = hh,
    };
    c->resize_img_delta_x = CropImage(h->buffer, xrect);
    c->t_resize_img_delta_x = CropTexture(h->t_buffer, xrect);
  }

  if (deltay < 0) {
    RectangleInt yrect = {
        .x = 0,
        .y = hh + deltay,
        .width = w,
        .height = -deltay,
    };
    c->resize_img_delta_y = CropImage(h->buffer, yrect);
    c->t_resize_img_delta_y = CropTexture(h->t_buffer, yrect);
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
  assert(h->selbuffer.width > 0);
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
  if (h->selbuffer.width == 0) {
    return;
  }
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_CREATE);
  // makes the selection be empty after modif
  c->sel_rect = (RectangleInt){0};
  c->data_after = CloneImage(h->selbuffer);
  c->t_data_after = CloneTexture(h->t_selbuffer);
  c->offset = (Vector2Int){.x = h->seloff.x, .y = h->seloff.y};
  // I leave data_before as empty showing it has no impact on image
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistActSelFill(Hist* h, Color fill_color) {
  if (h->selbuffer.width == 0) {
    return;
  }
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_CREATE);
  c->data_after = CloneImage(h->selbuffer);
  c->t_data_after = CloneTexture(h->t_selbuffer);
  c->offset.x = h->seloff.x;
  c->offset.y = h->seloff.y;
  // I leave data_before as empty showing it has no impact on image

  c->next = h->undo_hist;
  c->paste_data =
      GenImageFilled(h->selbuffer.width, h->selbuffer.height, fill_color);
  c->t_paste_data = CloneTextureFromImage(c->paste_data);
  c->sel_rect = (RectangleInt){
      .x = h->seloff.x,
      .y = h->seloff.y,
      .width = h->selbuffer.width,
      .height = h->selbuffer.height,
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
  c->data_before = CropImage(h->buffer, r);
  c->t_data_before = CropTexture(h->t_buffer, r);
  c->data_after = img;
  c->t_data_after = CloneTextureFromImage(img);
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistActCommitSel(Hist* h, RectangleInt tool_rect) {
  Cmd* c = LoadCmd(TOOL_SEL, ACTION_SEL_CREATE);
  c->sel_rect = GetCollisionRecInt(tool_rect, GetImageRect(h->buffer));
  // We consider rects of size 1 to be empty.
  bool is_empty = c->sel_rect.width * c->sel_rect.height <= 1;
  if (is_empty) {
    c->sel_rect = (RectangleInt){0};
  }
  if (h->selbuffer.width > 0) {
    c->data_after = CloneImage(h->selbuffer);
    c->t_data_after = CloneTexture(h->t_selbuffer);
    RectangleInt r = {
        .x = h->seloff.x,
        .y = h->seloff.y,
        .width = h->selbuffer.width,
        .height = h->selbuffer.height,
    };
    RectangleInt cropped_r = GetCollisionRecInt(r, GetImageRect(h->buffer));
    if (cropped_r.width * cropped_r.height > 0) {
      c->data_before = CropImage(h->buffer, cropped_r);
      c->t_data_before = CropTexture(h->t_buffer, cropped_r);
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

  if (h->selbuffer.width > 0) {
    c->data_after = CloneImage(h->selbuffer);
    c->t_data_after = CloneTexture(h->t_selbuffer);
    RectangleInt r = {
        .x = h->seloff.x,
        .y = h->seloff.y,
        .width = h->selbuffer.width,
        .height = h->selbuffer.height,
    };
    RectangleInt cropped_r = GetCollisionRecInt(r, GetImageRect(h->buffer));
    if (cropped_r.width * cropped_r.height > 0) {
      c->data_before = CropImage(h->buffer, cropped_r);
      c->t_data_before = CropTexture(h->t_buffer, cropped_r);
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
  c->t_paste_data = CloneTextureFromImage(img);
  c->next = h->undo_hist;
  h->undo_hist = c;
  HistEmptyRedo(h);
  CmdDo(c, h);
}

void HistSetBuffer(Hist* h, Image buffer) {
  HistClearBufer(h);
  h->buffer = buffer;
  // Here is the case where the loading has failed.
  if (h->buffer.width == 0) {
    int ww = 1024;
    int hh = 1024;
    h->buffer = GenImageFilled(ww, hh, BLANK);
  }
  ImageRemoveBlacks(&h->buffer);
  h->t_buffer = CloneTextureFromImage(h->buffer);
  h->dirty = false;
  HistResetUndoHistory(h);
}

void HistNewBuffer(Hist* h, int bw, int bh) {
  Image img = GenImageFilled(bw, bh, BLANK);
  HistSetBuffer(h, img);
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
    UnloadRenderTexture(c->t_data_after);
  }
  if (c->data_before.width > 0) {
    UnloadImage(c->data_before);
    UnloadRenderTexture(c->t_data_before);
  }
  if (c->paste_data.width > 0) {
    UnloadImage(c->paste_data);
    UnloadRenderTexture(c->t_paste_data);
  }
  if (c->resize_img_delta_x.width > 0) {
    UnloadImage(c->resize_img_delta_x);
    UnloadRenderTexture(c->t_resize_img_delta_x);
  }
  if (c->resize_img_delta_y.width > 0) {
    UnloadImage(c->resize_img_delta_y);
    UnloadRenderTexture(c->t_resize_img_delta_y);
  }
  free(c);
}

RectangleInt HistGetBufferRect(Hist* h) { return GetImageRect(h->buffer); }
