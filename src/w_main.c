#include "w_main.h"

#include "api.h"
#include "colors.h"
#include "filedialog.h"
#include "font.h"
#include "minimap.h"
#include "msg.h"
#include "paint.h"
#include "raylib.h"
#include "rlgl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tiling.h"
#include "utils.h"
#include "w_about.h"
#include "w_dialog.h"
#include "w_levels.h"
#include "w_text.h"
#include "w_tutorial.h"
#include "widgets.h"

#define MSG_DURATION 2

static struct {
  bool inited;
  // Image Drawing target, has same size as the original image
  RenderTexture2D img_target_tex;
  // Image overlay where the lua stuff can draw to.
  RenderTexture2D level_overlay_tex;
  Vector2 target_pos;
  Rectangle color_btn[64];
  Rectangle fg_color_rect;
  Color palette[64];
  int num_colors;
  // Currently open filename.
  // Only relevant when the user has opened a file, or if he has saved one.
  // When user clicks on "save", that's the fname used.
  char* fname;
  bool mouse_on_target;
  Paint ca;
  int header_size;
  int bottom_size;

  // Topbar buttons
  Btn btn_new;
  Btn btn_open;
  Btn btn_save;
  Btn btn_saveas;
  Btn btn_about;
  Btn btn_exit;

  // Tools buttons
  Btn btn_simu;
  Btn btn_brush;
  Btn btn_text;
  Btn btn_line;
  Btn btn_picker;
  Btn btn_bucket;
  Btn btn_marquee;
  Btn btn_rotate;
  Btn btn_fill;
  Btn btn_fliph;
  Btn btn_flipv;
  Btn btn_sel_open;
  Btn btn_sel_save;
  Btn btn_clockopt[6];

  // Right side buttons
  Btn btn_challenge;
  Btn btn_tutorial;
  Btn btn_minimap;

  Minimap minimap;
  bool show_minimap;
} C = {0};

static void MainInit(Ui* ui);
static void MainUpdateTitle();
static void MainUpdateLayout(Ui* ui);
static void MainUpdateViewport(Ui* ui);
static void MainDrawStatusBar(Ui* ui);
static void MainDrawErrorMessage(Ui* ui);
static void MainDrawMouseExtra(Ui* ui);
static void MainCheckFileDrop();
static void MainUpdateWidgets();
static void MainUpdateControls(Ui* ui);
static void MainUpdateHud(Ui* ui);
static void MainCheckWindowResize(Ui* ui);
static void MainNewFile(Ui* ui);
static void MainOpenFileModal(Ui* ui);
static bool RectHover(Rectangle hitbox, Vector2 pos);
static bool MainGetIsCursorInTargeTimage();
static char* MainGetFilename();
static RectangleInt MainGetTargetRegion();
static void MainLoadImageFromPath(const char* path);
static void MainOpenSelection(Ui* ui);
static void MainSaveSelection(Ui* ui);

static bool RectHover(Rectangle hitbox, Vector2 pos) {
  return CheckCollisionPointRec(pos, hitbox);
}

void MainOpen(Ui* ui) {
  ui->window = WINDOW_MAIN;
  if (!C.inited) {
    C.inited = true;
    MainInit(ui);
  }
}

void MainInit(Ui* ui) {
  C.minimap.s = 2;
  C.minimap.hitbox = (Rectangle){0, 0, 2 * 17 * 2, 2 * 17 * 2};
  C.palette[0] = WHITE;
  C.palette[1] = BLACK;
  C.palette[2] = RED;
  C.palette[3] = GREEN;
  C.palette[4] = BLUE;
  C.palette[5] = YELLOW;
  C.palette[6] = PURPLE;
  C.palette[7] = SKYBLUE;
  C.palette[8] = DARKGREEN;
  C.palette[9] = PINK;
  C.palette[10] = ORANGE;
  C.palette[11] = DARKPURPLE;
  C.palette[12] = VIOLET;
  C.palette[13] = BEIGE;
  C.palette[14] = DARKBLUE;
  C.palette[15] = LIME;
  C.num_colors = 16;
  C.header_size = 24 * ui->scale;
  C.bottom_size = 3 * 17 * 1 * ui->scale;

#ifdef WEB
  C.btn_challenge.disabled = true;
  C.btn_tutorial.disabled = true;
  C.btn_new.disabled = true;
  C.btn_save.disabled = true;
  C.btn_saveas.disabled = true;
  C.btn_open.disabled = true;
  C.btn_exit.disabled = true;
#endif
  PaintLoad(&C.ca);
  LevelOptions* opt = ApiGetLevelOptions();
  if (!opt->startup_image_path) {
    // This way here is not the same as MainLoadImageFromPath: when we load
    // image this way, the image's path is not associated to it, so the user
    // can't override the initial tutorial image directly by pressing "save".
    Image img = LoadImage("../assets/tutorial.png");
    PaintLoadImage(&C.ca, img);
  } else {
    MainLoadImageFromPath(opt->startup_image_path);
  }

  MainUpdateViewport(ui);
  MainUpdateTitle();
  MainUpdateWidgets();
}

void MainUpdate(Ui* ui) {
  MainUpdateViewport(ui);
  MainCheckFileDrop();
  MsgUpdate();
  C.mouse_on_target = false;
  MainUpdateControls(ui);
  if (C.ca.mode == MODE_SIMU && C.ca.s.status == SIMU_STATUS_OK) {
    float delta = GetFrameTime();
    PaintUpdateSimu(&C.ca, delta);
  }
  MainUpdateWidgets();
  PaintRender(&C.ca);
  UpdateTexture(C.img_target_tex.texture, GetPixels(C.ca.tmp_render));
  BeginTextureMode(C.level_overlay_tex);
  ClearBackground(BLANK);
  EndTextureMode();
  ApiOnLevelDraw(C.level_overlay_tex, C.ca.camera_x, C.ca.camera_y,
                 C.ca.camera_s);
}

void MainUpdateControls(Ui* ui) {
  MainUpdateHud(ui);
#ifndef WEB
  if (IsKeyPressed(KEY_TAB)) {
    TutorialOpen(ui);
  }
#endif
  if (IsKeyPressed(KEY_K)) {
    C.show_minimap = !C.show_minimap;
  }

  if (IsKeyPressed(KEY_S) && IsKeyDown(KEY_LEFT_CONTROL)) {
    MainOnSaveClick(ui, false);
  }

  if (C.show_minimap) {
    int target_w = C.img_target_tex.texture.width;
    int target_h = C.img_target_tex.texture.height;
    MinimapUpdate(&C.minimap, &C.ca, ui, target_w, target_h);
  }
  bool mouse_on_target = MainGetIsCursorInTargeTimage() && ui->hit_count == 0;
  C.mouse_on_target = mouse_on_target;
  PaintUpdatePixelPosition(&C.ca);
  PaintEnforceMouseOnImageIfNeed(&C.ca);
  if (C.mouse_on_target && ui->hit_count == 0) {
    PaintHandleWheelZoom(&C.ca);
    PaintHandleCameraMovement(&C.ca);
    if (PaintGetMode(&C.ca) == MODE_EDIT) {
      ToolType tool = PaintGetDisplayTool(&C.ca);
      switch (tool) {
        case TOOL_SEL: {
          bool move =
              PaintGetMouseOverSel(&C.ca) || PaintGetIsToolSelMoving(&C.ca);
          if (move) {
            ui->cursor = MOUSE_MOVE;
          } else {
            ui->cursor = MOUSE_SELECTION;
          }
          break;
        };
        case TOOL_BRUSH:
        case TOOL_LINE: {
          ui->cursor = MOUSE_PEN;
          break;
        }
        case TOOL_BUCKET: {
          ui->cursor = MOUSE_BUCKET;
          break;
        }
        case TOOL_PICKER: {
          ui->cursor = MOUSE_PICKER;
          break;
        }
        default: {
          ui->cursor = MOUSE_ARROW;
          break;
        }
      }
    } else {
      int cs;
      PaintGetSimuPixelToggleState(&C.ca, &cs);
      if (cs == 0 || cs == 1) {
        ui->cursor = MOUSE_POINTER;
      } else {
        ui->cursor = MOUSE_ARROW;
      }
    }
  } else {
    ui->cursor = MOUSE_ARROW;
  }
  if (C.ca.mode == MODE_EDIT) {
    if (IsKeyPressed(KEY_T)) {
      TextModalOpen(ui);
    }
  }
  PaintHandleMouse(&C.ca, C.mouse_on_target);
  PaintHandleKeys(&C.ca);
}

void MainOpenSelection(Ui* ui) {
  ModalResult mr = ModalOpenFile(NULL);
  if (mr.ok) {
    Image img = LoadImage(mr.path);
    PaintPasteImage(&C.ca, img);
    free(mr.path);
  } else if (mr.cancel) {
  } else {
    char txt[500];
    sprintf(txt, "ERROR: %s\n", mr.error_msg);
    MsgAdd(txt, MSG_DURATION);
  }
}

void MainSaveSelection(Ui* ui) {
  ModalResult mr = ModalSaveFile(NULL, NULL);
  if (mr.cancel) {
    return;
  } else if (!mr.ok) {
    char txt[500];
    sprintf(txt, "ERROR: %s\n", mr.error_msg);
    MsgAdd(txt, MSG_DURATION);
    return;
  }

  if (mr.path && mr.ok) {
    Image out = CloneImage(PaintGetSelBuffer(&C.ca));
    // Before saving, add black pixels back
    ImageAddBlacks(out);
    if (!ExportImage(out, mr.path)) {
      MsgAdd("ERROR: Could not save selection ...", MSG_DURATION);
      return;
    }
    UnloadImage(out);
    MsgAdd("Selection Image Saved.", MSG_DURATION);
    return;
  }
}

void MainUpdateHud(Ui* ui) {
  if (BtnUpdate(&C.btn_new, ui)) MainAskForSaveAndProceed(ui, MainNewFile);
  if (BtnUpdate(&C.btn_open, ui))
    MainAskForSaveAndProceed(ui, MainOpenFileModal);
  if (BtnUpdate(&C.btn_save, ui)) MainOnSaveClick(ui, false);
  if (BtnUpdate(&C.btn_saveas, ui)) MainOnSaveClick(ui, true);
  if (BtnUpdate(&C.btn_about, ui)) AboutOpen(ui);
  if (BtnUpdate(&C.btn_exit, ui)) ui->close_requested = true;

  if (BtnUpdate(&C.btn_sel_open, ui)) MainOpenSelection(ui);
  if (BtnUpdate(&C.btn_sel_save, ui)) MainSaveSelection(ui);

  if (BtnUpdate(&C.btn_line, ui)) PaintSetTool(&C.ca, TOOL_LINE);
  if (BtnUpdate(&C.btn_brush, ui)) PaintSetTool(&C.ca, TOOL_BRUSH);
  if (BtnUpdate(&C.btn_marquee, ui)) PaintSetTool(&C.ca, TOOL_SEL);
  if (BtnUpdate(&C.btn_text, ui)) TextModalOpen(ui);
  if (BtnUpdate(&C.btn_bucket, ui)) PaintSetTool(&C.ca, TOOL_BUCKET);
  if (BtnUpdate(&C.btn_picker, ui)) PaintSetTool(&C.ca, TOOL_PICKER);

  if (BtnUpdate(&C.btn_rotate, ui)) PaintActSelRot(&C.ca);
  if (BtnUpdate(&C.btn_fliph, ui)) PaintActSelFlipH(&C.ca);
  if (BtnUpdate(&C.btn_flipv, ui)) PaintActSelFlipV(&C.ca);
  if (BtnUpdate(&C.btn_fill, ui)) PaintActSelFill(&C.ca);

  if (BtnUpdate(&C.btn_clockopt[0], ui)) PaintSetClockSpeed(&C.ca, 0);
  if (BtnUpdate(&C.btn_clockopt[1], ui)) PaintSetClockSpeed(&C.ca, 1);
  if (BtnUpdate(&C.btn_clockopt[2], ui)) PaintSetClockSpeed(&C.ca, 2);
  if (BtnUpdate(&C.btn_clockopt[3], ui)) PaintSetClockSpeed(&C.ca, 3);
  if (BtnUpdate(&C.btn_clockopt[4], ui)) PaintSetClockSpeed(&C.ca, 4);
  if (BtnUpdate(&C.btn_clockopt[5], ui)) PaintSetClockSpeed(&C.ca, 5);

  if (BtnUpdate(&C.btn_simu, ui)) PaintToggleSimu(&C.ca);
#ifndef WEB
  if (BtnUpdate(&C.btn_challenge, ui)) LevelsOpen(ui);
  if (BtnUpdate(&C.btn_tutorial, ui)) TutorialOpen(ui);
#endif
  if (BtnUpdate(&C.btn_minimap, ui)) C.show_minimap = !C.show_minimap;

  Vector2 pos = GetMousePosition();
  if (RectHover(C.fg_color_rect, pos)) {
    ui->hit_count++;
  }
  for (int i = 0; i < C.num_colors; i++) {
    if (RectHover(C.color_btn[i], pos)) {
      ui->hit_count++;
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PaintSetColor(&C.ca, C.palette[i]);
      }
    }
  }
}

void MainDraw(Ui* ui) {
  ClearBackground(BLANK);
  DrawDefaultTiledScreen(ui);

  // Draws target
  {
    Rectangle source = {
        .x = 0,
        .y = 0,
        .width = C.img_target_tex.texture.width,
        .height = C.img_target_tex.texture.height,
    };
    Rectangle source_inverted = {
        .x = 0,
        .y = 0,
        .width = source.width,
        .height = -source.height,
    };
    Rectangle target = {
        .x = C.target_pos.x,
        .y = C.target_pos.y,
        .width = source.width,
        .height = source.height,
    };
    DrawRectangle(C.target_pos.x, C.target_pos.y, source.width, source.height,
                  BLACK);
    DrawTexturePro(C.img_target_tex.texture, source, target,
                   (Vector2){.x = 0, .y = 0}, 0.0f, WHITE);

    // Drawing the text in the resize rectangle of the image.
    // I'm drawing here and not in the img because I want to scale the font.
    // When it's dragging, it displays the image size.
    if (C.ca.resize_hovered || C.ca.resize_pressed) {
      ui->cursor = MOUSE_RESIZE;
      BeginScissorMode(C.target_pos.x, C.target_pos.y,
                       C.img_target_tex.texture.width,
                       C.img_target_tex.texture.height);
      rlPushMatrix();
      rlTranslatef(C.target_pos.x, C.target_pos.y, 0);
      rlTranslatef(C.ca.camera_x, C.ca.camera_y, 0);
      rlScalef(C.ca.camera_s, C.ca.camera_s, 1);

      Image img = PaintGetEditImage(&C.ca);
      if (C.ca.resize_pressed) {
        rlTranslatef(C.ca.resize_region.width, C.ca.resize_region.height, 0);
      } else {
        rlTranslatef(img.width, img.height, 0);
      }

      rlScalef(1 / C.ca.camera_s, 1 / C.ca.camera_s, 1);
      rlScalef(2, 2, 1);
      rlTranslatef(10, -14, 0);

      char txt[100];
      if (C.ca.resize_pressed) {
        int w = C.ca.resize_region.width;
        int h = C.ca.resize_region.height;
        sprintf(txt, "%d x %d", w, h);
      } else {
        sprintf(txt, "Drag to resize");
      }
      FontDrawTexture(txt, 0, 0, WHITE);
      rlPopMatrix();
      EndScissorMode();
    }

    DrawTexturePro(C.level_overlay_tex.texture, source_inverted, target,
                   (Vector2){.x = 0, .y = 0}, 0.0f, WHITE);
  }

  Rectangle inner_content = {
      C.target_pos.x,
      C.target_pos.y,
      C.level_overlay_tex.texture.width,
      C.level_overlay_tex.texture.height,
  };
  DrawDefaultTiledFrame(ui, inner_content);

  LevelOptions* co = ApiGetLevelOptions();
  LevelDesc* cd = ApiGetLevelDesc();

  BtnDrawIcon(&C.btn_new, 2, ui->sprites, rect_new);
  BtnDrawIcon(&C.btn_open, 2, ui->sprites, rect_open);
  BtnDrawIcon(&C.btn_save, 2, ui->sprites, rect_save);
  BtnDrawIcon(&C.btn_saveas, 2, ui->sprites, rect_saveas);
  BtnDrawIcon(&C.btn_about, 2, ui->sprites, rect_info);
  BtnDrawIcon(&C.btn_exit, 2, ui->sprites, rect_exit);

  int bscale = ui->scale;
  bool simu_on = PaintGetMode(&C.ca) == MODE_SIMU;
  BtnDrawIcon(&C.btn_simu, bscale, ui->sprites,
              simu_on ? rect_stop : rect_start);
#ifndef WEB
  C.btn_challenge.disabled = PaintGetMode(&C.ca) != MODE_EDIT;
  BtnDrawIcon(&C.btn_challenge, bscale, co->options[cd->ilevel].icon.tex,
              co->options[cd->ilevel].icon.region);
#else
  BtnDrawIcon(&C.btn_challenge, bscale, ui->sprites, rect_sandbox);
#endif

  BtnDrawText(&C.btn_tutorial, bscale, "Tutorial");

  BtnDrawIcon(&C.btn_minimap, bscale, ui->sprites, rect_map);
  bool color_disabled = PaintGetMode(&C.ca) != MODE_EDIT;
  BtnDrawColor(ui, C.fg_color_rect, PaintGetColor(&C.ca), false,
               color_disabled);
  BtnDrawIcon(&C.btn_line, bscale, ui->sprites, rect_line);
  BtnDrawIcon(&C.btn_brush, bscale, ui->sprites, rect_brush);
  BtnDrawIcon(&C.btn_picker, bscale, ui->sprites, rect_picker);
  BtnDrawIcon(&C.btn_bucket, bscale, ui->sprites, rect_bucket);
  BtnDrawIcon(&C.btn_marquee, bscale, ui->sprites, rect_marquee);
  BtnDrawIcon(&C.btn_text, bscale, ui->sprites, rect_text);
  BtnDrawIcon(&C.btn_rotate, bscale, ui->sprites, rect_rot);
  BtnDrawIcon(&C.btn_fliph, bscale, ui->sprites, rect_fliph);
  BtnDrawIcon(&C.btn_flipv, bscale, ui->sprites, rect_flipv);
  BtnDrawIcon(&C.btn_fill, bscale, ui->sprites, rect_fill);
  BtnDrawIcon(&C.btn_sel_open, bscale, ui->sprites, rect_sel_open);
  BtnDrawIcon(&C.btn_sel_save, bscale, ui->sprites, rect_sel_save);

  BtnDrawIcon(&C.btn_clockopt[0], bscale, ui->sprites, rect_hz0);
  BtnDrawIcon(&C.btn_clockopt[1], bscale, ui->sprites, rect_hz1);
  BtnDrawIcon(&C.btn_clockopt[2], bscale, ui->sprites, rect_hz4);
  BtnDrawIcon(&C.btn_clockopt[3], bscale, ui->sprites, rect_hz16);
  BtnDrawIcon(&C.btn_clockopt[4], bscale, ui->sprites, rect_hz64);
  BtnDrawIcon(&C.btn_clockopt[5], bscale, ui->sprites, rect_hz1k);

  MsgDraw(ui);
  for (int i = 0; i < C.num_colors; i++) {
    BtnDrawColor(ui, C.color_btn[i], C.palette[i],
                 COLOR_EQ(C.palette[i], PaintGetColor(&C.ca)), color_disabled);
  }

  if (C.ca.mode == MODE_SIMU) {
    if (C.ca.s.status != SIMU_STATUS_OK) {
      MainDrawErrorMessage(ui);
    }
  }

  // We only draw the legends if this window is the active window.
  if (ui->window == WINDOW_MAIN) {
    BtnDrawLegend(&C.btn_new, bscale, "New Image");
    BtnDrawLegend(&C.btn_open, bscale, "Open Image");
    BtnDrawLegend(&C.btn_save, bscale, "Save Image (C-S)");
    BtnDrawLegend(&C.btn_saveas, bscale, "Save Image As...");
    BtnDrawLegend(&C.btn_about, bscale, "About Circuit Artist");
    BtnDrawLegend(&C.btn_exit, bscale, "Exit");

    BtnDrawLegend(&C.btn_sel_open, bscale, "Import Selection from Image");
    BtnDrawLegend(&C.btn_sel_save, bscale, "Save Selection as Image");

    BtnDrawLegend(
        &C.btn_simu, bscale,
        simu_on ? "Stop Simulation (SPACE)" : "Start Simulation (SPACE)");
    BtnDrawLegend(&C.btn_brush, bscale,
                  "Brush tool (B)\nLeft mouse button: draw\nRight mouse "
                  "button: erase\nPress (ALT) to pick color.");
    BtnDrawLegend(&C.btn_line, bscale,
                  "Line tool (L)\nType (NUMBER) to change line size.\nPress "
                  "(SHIFT) while drawing to add corner to start of "
                  "line.\nPress (CTRL) while drawing to add corner to end of "
                  "line.\nLeft mouse button: Draw\nRight mouse button: "
                  "Erase\nPress (ALT) to pick color.");
    BtnDrawLegend(&C.btn_bucket, bscale,
                  "Wire Fill (G)\nLeft mouse button: regular color "
                  "fill.\nRight mouse button: black color fill (erase).\nPress "
                  "(ALT) to pick color.");
    BtnDrawLegend(&C.btn_picker, bscale, "Color Picker (I)");
    BtnDrawLegend(
        &C.btn_marquee, bscale,
        "Selection tool (M)\nHold (LEFT CTRL) and drag to create a copy of the "
        "selection.\nHold (SHIFT) while dragging to force a strict X or Y "
        "movement.\nUse (ARROW)s to move a selection by single pixels. "
        "(CTRL+ARROW)s to move by 4 pixels. \n(C-C)/(C-V) for copy/paste "
        "selection (copies/pastes to/from clipboard).\n(DELETE) or (BACKSPACE) "
        "to delete selection.\n (ESCAPE) to deselect.");
    BtnDrawLegend(&C.btn_text, bscale, "Add Text (T)");
    BtnDrawLegend(&C.btn_fliph, bscale, "Flip selection horizontally (H)");
    BtnDrawLegend(&C.btn_flipv, bscale, "Flip selection vertically (V)");
    BtnDrawLegend(&C.btn_rotate, bscale, "Rotate selection (R)");
    BtnDrawLegend(&C.btn_fill, bscale, "Fill selection (F)");
    BtnDrawLegend(&C.btn_challenge, bscale, "Select Level");
    BtnDrawLegend(&C.btn_tutorial, bscale,
                  "Tutorial (TAB)\n`-` Describes core game concepts and "
                  "mechanics.\n`-` Introduces a "
                  "number of digital logic concepts and components.");
    BtnDrawLegend(&C.btn_minimap, bscale, "Show/Hide Minimap (K)");

    BtnDrawLegend(&C.btn_clockopt[0], bscale, "0 Hz Simulation");
    BtnDrawLegend(&C.btn_clockopt[1], bscale, "1 Hz Simulation");
    BtnDrawLegend(&C.btn_clockopt[2], bscale, "4 Hz Simulation");
    BtnDrawLegend(&C.btn_clockopt[3], bscale, "16 Hz Simulation");
    BtnDrawLegend(&C.btn_clockopt[4], bscale, "64 Hz Simulation");
    BtnDrawLegend(&C.btn_clockopt[5], bscale, "1024 Hz Simulation");
  }

  if (C.show_minimap) {
    MinimapDraw(&C.minimap);
  }
  MainDrawStatusBar(ui);
  if (ui->window == WINDOW_MAIN) {
    MainDrawMouseExtra(ui);
  }
}

void MainSetPaletteFromImage(Image img) {
  int w = img.width;
  int nc = w / 8;
  if (nc > 64) nc = 64;
  int d = 8;
  Color* pixels = (Color*)img.data;
  for (int i = 0; i < nc; i++) {
    Color c = pixels[i * d];
    c.a = 255;
    C.palette[i] = c;
  }
  C.num_colors = nc;
}

void MainUpdateLayout(Ui* ui) {
  int s = ui->scale;
  {
    int y0 = 4 * s;
    int x0 = 4 * s;
    int x1 = x0 + 18 * s;
    int x2 = x1 + 18 * s;
    int x3 = x2 + 18 * s;
    int x4 = x3 + 18 * s;
    int x5 = x4 + 18 * s;
    int x6 = x5 + 18 * s;
    int x7 = x6 + 18 * s;
    int x8 = x7 + 18 * s;
    int x9 = x8 + 18 * s;
    int x10 = x9 + 18 * s;
    int bw = 17 * s;
    int bh = 17 * s;
    C.btn_new.hitbox = (Rectangle){x0, y0, bw, bh};
    C.btn_open.hitbox = (Rectangle){x1, y0, bw, bh};
    C.btn_save.hitbox = (Rectangle){x2, y0, bw, bh};
    C.btn_saveas.hitbox = (Rectangle){x3, y0, bw, bh};
    C.btn_about.hitbox = (Rectangle){x4, y0, bw, bh};
    C.btn_exit.hitbox = (Rectangle){x5, y0, bw, bh};

    C.btn_tutorial.hitbox = (Rectangle){x8, y0, 3 * bw, bh};
  }

  int sh = GetScreenHeight() / s;
  {
    int bx0 = 4 * s;
    int by0 = 4 + 2 * 18 * s;
    int by0_simu = by0;
    C.btn_simu.hitbox = (Rectangle){bx0, by0, (2 * 17 + 1) * s, 17 * s};
    by0 = by0 + 18 * s;
    by0 = by0 + 4 * s;
    int by1 = by0 + 18 * s;
    int by2 = by1 + 18 * s;
    int by3 = by2 + 18 * s + 4 * s;
    int by4 = by3 + 18 * s;
    int by5 = by4 + 18 * s;
    int by5b = by4 + 18 * s + 4 * s;

    int bx1 = bx0 + 18 * s;
    int bw = 17 * s;
    int bh = 17 * s;

    C.btn_brush.hitbox = (Rectangle){bx0, by0, bw, bh};
    C.btn_line.hitbox = (Rectangle){bx1, by0, bw, bh};
    C.btn_marquee.hitbox = (Rectangle){bx0, by1, bw, bh};
    C.btn_text.hitbox = (Rectangle){bx1, by1, bw, bh};
    C.btn_bucket.hitbox = (Rectangle){bx0, by2, bw, bh};
    C.btn_picker.hitbox = (Rectangle){bx1, by2, bw, bh};

    C.btn_fliph.hitbox = (Rectangle){bx0, by3, bw, bh};
    C.btn_flipv.hitbox = (Rectangle){bx1, by3, bw, bh};
    C.btn_rotate.hitbox = (Rectangle){bx0, by4, bw, bh};
    C.btn_fill.hitbox = (Rectangle){bx1, by4, bw, bh};
    C.btn_sel_open.hitbox = (Rectangle){bx0, by5b, bw, bh};
    C.btn_sel_save.hitbox = (Rectangle){bx1, by5b, bw, bh};

    C.btn_clockopt[0].hitbox = (Rectangle){bx0, by3, bw, bh};
    C.btn_clockopt[1].hitbox = (Rectangle){bx1, by3, bw, bh};
    C.btn_clockopt[2].hitbox = (Rectangle){bx0, by4, bw, bh};
    C.btn_clockopt[3].hitbox = (Rectangle){bx1, by4, bw, bh};
    C.btn_clockopt[4].hitbox = (Rectangle){bx0, by5, bw, bh};
    C.btn_clockopt[5].hitbox = (Rectangle){bx1, by5, bw, bh};

    int cy = (sh - 2 * 18 - 4) * s;
    int cx = 4 * s + 35 * s + 4 * s;
    C.fg_color_rect = (Rectangle){
        .x = cx,
        .y = cy,
        .width = 2 * 17 * s,
        .height = 2 * 17 * s,
    };
    for (int i = 0; i < 64; i++) {
      int bx = i / 2;
      int by = i % 2;
      C.color_btn[i] = (Rectangle){
          .x = cx + (2 + bx) * 17 * s,
          .y = cy + (by) * (17 * s),
          .width = 1 * 17 * s,
          .height = 1 * 17 * s,
      };
    }

    int chax = GetScreenWidth() - 6 * s - 35 * s;
    C.btn_challenge.hitbox = (Rectangle){chax, by0_simu, 35 * s, s * 35};

    int mw = 6 * 17 * s;
    int yy = GetScreenHeight() - mw - 4 * s;
    C.btn_minimap.hitbox = (Rectangle){chax, yy - 4 * s - bh, bw, bh};
  }
}

void MainUpdateViewport(Ui* ui) {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  MainUpdateLayout(ui);
  int pad = 4;
  int scale = 2;
  int tt = 7 * scale;
  C.target_pos = (Vector2){
      .x = pad + 42 * scale + tt,
      .y = C.header_size + 2 * scale + tt,
  };
  int tgt_size_x = sw - C.target_pos.x - pad - tt - 8 * scale - 35 * scale;
  int tgt_size_y = sh - C.bottom_size - C.header_size - tt;
  // Avoids crashing when window is too small
  const int min_tgt_size = 32;
  tgt_size_x = MaxInt(min_tgt_size, tgt_size_x);
  tgt_size_y = MaxInt(min_tgt_size, tgt_size_y);
  if (tgt_size_x != C.img_target_tex.texture.width ||
      tgt_size_y != C.img_target_tex.texture.height) {
    if (C.img_target_tex.texture.width > 0) {
      UnloadRenderTexture(C.img_target_tex);
      UnloadRenderTexture(C.level_overlay_tex);
    }
    C.img_target_tex = LoadRenderTexture(tgt_size_x, tgt_size_y);
    C.level_overlay_tex = LoadRenderTexture(tgt_size_x, tgt_size_y);
  }

  int s = ui->scale;
  C.minimap.s = s;
  int mw = 6 * 17 * s;
  C.minimap.hitbox = (Rectangle){
      GetScreenWidth() - mw - 4 * s,
      GetScreenHeight() - mw - 4 * s,
      mw,
      mw,
  };
}

char* MainGetFilename() {
  if (C.fname) {
    return C.fname;
  }
  return "Untitled";
}

void MainUpdateTitle() {
  const char* fname = GetFileName(MainGetFilename());
  char tmp[400];
  if (strlen(fname) >= 400) {
    sprintf(tmp, "%.*s ... - Circuit Artist", 400, fname);
  } else {
    sprintf(tmp, "%s - Circuit Artist", fname);
  }
  SetWindowTitle(tmp);
}

void MainDrawErrorMessage(Ui* ui) {
  int x = C.target_pos.x;
  int y = C.target_pos.y;

  rlPushMatrix();
  rlTranslatef(x + 20, y + 20, 0);
  rlScalef(ui->scale, ui->scale, 1);
  char txt[500];
  sprintf(txt, "ERROR: %s", C.ca.s.crash_reason);
  int tw = GetRenderedTextSize(txt).x;
  int lh = GetFontLineHeight();
  int th = 17;
  int dy = (th - lh) / 2;
  int pad = 10;
  DrawRectangle(0, 0, tw + 2 * pad, th + 2 * pad, RED);
  DrawTextBox(txt, (Rectangle){pad, pad + dy, 1000, 0}, WHITE, NULL);
  rlPopMatrix();
}

void MainDrawStatusBar(Ui* ui) {
  int ui_scale = ui->scale;
  Color tc = WHITE;
  int num_lines = 8;
  int yc1 = (C.target_pos.y + C.img_target_tex.texture.height) / ui_scale -
            17 * num_lines;
  int xc = 1 * 17 + C.target_pos.x / ui_scale;
  int yc2 = yc1 + 17;
  int yc3 = yc2 + 17;
  int yc4 = yc3 + 17;
  int yc5 = yc4 + 17;
  int yc6 = yc5 + 17;
  int yc7 = yc6 + 17;
  int yc8 = yc7 + 17;
  rlPushMatrix();
  Color c2 = GetLutColor(COLOR_BTN2);

  tc = c2;
  rlScalef(ui_scale, ui_scale, 1);
  char txt[500];
  Color bg = BLACK;
  if (MainGetIsCursorInTargeTimage()) {
    sprintf(txt, "X: %d Y: %d", C.ca.pixel_cursor_x, C.ca.pixel_cursor_y);
    FontDrawTextureOutlined(txt, xc, yc1, tc, bg);
    int tx = C.ca.tool_end_x - C.ca.tool_start_x;
    int ty = C.ca.tool_end_y - C.ca.tool_start_y;
    tx = tx < 0 ? -tx : tx;
    ty = ty < 0 ? -ty : ty;
    int tool = PaintGetTool(&C.ca);
    if (C.ca.mode == MODE_EDIT) {
      if (C.ca.tool_pressed && !PaintGetIsToolSelMoving(&C.ca) &&
          tool == TOOL_SEL) {
        sprintf(txt, "w: %d h: %d (%d pixels)", tx + 1, ty + 1,
                (tx + 1) * (ty + 1));
        FontDrawTextureOutlined(txt, xc, yc2, tc, bg);
      } else if (PaintGetHasSelection(&C.ca)) {
        Image selbuffer = PaintGetSelBuffer(&C.ca);
        int sw = selbuffer.width;
        int sh = selbuffer.height;
        sprintf(txt, "w: %d h: %d (%d pixels)", sw, sh, sw * sh);
        FontDrawTextureOutlined(txt, xc, yc2, tc, bg);
      }
    }
    int zoom = 100 * C.ca.camera_s;
    sprintf(txt, "Z: %d%%", zoom);
    FontDrawTextureOutlined(txt, xc, yc3, tc, bg);
  }
  if (C.ca.mode == MODE_SIMU) {
    if (C.ca.s.status == SIMU_STATUS_OK) {
      int num_nands = PaintGetNumNands(&C.ca);
      sprintf(txt, "NANDS: %d", num_nands);
      FontDrawTextureOutlined(txt, xc, yc4, tc, bg);

      int clock_count = C.ca.clock_count;
      sprintf(txt, "CLK: %d", clock_count);
      FontDrawTextureOutlined(txt, xc, yc5, tc, bg);

      int updates = C.ca.s.total_updates;
      sprintf(txt, "NAND Updates: %d", updates);
      FontDrawTextureOutlined(txt, xc, yc6, tc, bg);
    } else {
      sprintf(txt, "ERROR: %s", C.ca.s.crash_reason);
      FontDrawTextureOutlined(txt, xc, yc4, tc, bg);
    }
  }
  const char* fname = GetFileName(MainGetFilename());
  sprintf(txt, "[img] %s", fname);
  FontDrawTextureOutlined(txt, xc, yc7, tc, bg);
  Image img = PaintGetEditImage(&C.ca);
  sprintf(txt, "[img] w: %d h: %d", img.width, img.height);
  FontDrawTextureOutlined(txt, xc, yc8, tc, bg);
  rlPopMatrix();
}

RectangleInt MainGetTargetRegion() {
  return (RectangleInt){
      .x = C.target_pos.x,
      .y = C.target_pos.y,
      .width = C.img_target_tex.texture.width,
      .height = C.img_target_tex.texture.height,
  };
}

bool MainGetIsCursorInTargeTimage() {
  Vector2 pos = GetMousePosition();
  RectangleInt r = MainGetTargetRegion();
  Rectangle target_rect = {r.x, r.y, r.width, r.height};
  return CheckCollisionPointRec(pos, target_rect);
}

void MainDrawMouseExtra(Ui* ui) {
  if (C.ca.mode == MODE_EDIT && PaintGetTool(&C.ca) == TOOL_LINE &&
      ui->cursor == MOUSE_PEN) {
    Vector2 pos = GetMousePosition();
    int s = ui->scale;
    bool just_changed = PaintGetKeyLineWidthHasJustChanged(&C.ca);
    char txt[20];
    sprintf(txt, "w=%d", C.ca.line_tool_size);
    rlPushMatrix();
    rlTranslatef(pos.x + 8 * s, pos.y + 8 * s, 0);
    rlScalef(ui->scale, ui->scale, 1);
    FontDrawTexture(txt, 0, 0, just_changed ? YELLOW : WHITE);
    rlPopMatrix();
  }
}

void MainCheckFileDrop() {
  if (!IsFileDropped()) {
    return;
  }
  FilePathList path_list = LoadDroppedFiles();
  if (path_list.count > 0) {
    const char* fname = path_list.paths[0];
    Image img = LoadImage(fname);
    if (img.width > 0) {
      PaintPasteImage(&C.ca, img);
    }
  }
  UnloadDroppedFiles(path_list);
}

void MainUpdateWidgets() {
  bool ned = C.ca.mode != MODE_EDIT;
  int tool = PaintGetTool(&C.ca);
  C.btn_simu.disabled = (C.ca.mode != MODE_EDIT) && (C.ca.mode != MODE_SIMU);

  C.btn_brush.disabled = ned;
  C.btn_line.disabled = ned;
  C.btn_marquee.disabled = ned;
  C.btn_text.disabled = ned;
  C.btn_bucket.disabled = ned;
  C.btn_picker.disabled = ned;

  C.btn_brush.toggled = tool == TOOL_BRUSH;
  C.btn_line.toggled = tool == TOOL_LINE;
  C.btn_marquee.toggled = tool == TOOL_SEL;
  C.btn_bucket.toggled = tool == TOOL_BUCKET;
  C.btn_picker.toggled = tool == TOOL_PICKER;
  C.btn_minimap.toggled = C.show_minimap;

  int has_sel = PaintGetHasSelection(&C.ca) && tool == TOOL_SEL;
  C.btn_rotate.disabled = !has_sel || ned;
  C.btn_flipv.disabled = !has_sel || ned;
  C.btn_fliph.disabled = !has_sel || ned;
  C.btn_fill.disabled = !has_sel || ned;

  C.btn_sel_open.disabled = ned;
  C.btn_sel_save.disabled = !has_sel || ned;

  C.btn_rotate.hidden = (tool != TOOL_SEL) || ned;
  C.btn_flipv.hidden = (tool != TOOL_SEL) || ned;
  C.btn_fliph.hidden = (tool != TOOL_SEL) || ned;
  C.btn_fill.hidden = (tool != TOOL_SEL) || ned;

  C.btn_sel_open.hidden = (tool != TOOL_SEL) || ned;
  C.btn_sel_save.hidden = (tool != TOOL_SEL) || ned;

  for (int i = 0; i < 6; i++) {
    C.btn_clockopt[i].hidden = C.ca.mode != MODE_SIMU;
    C.btn_clockopt[i].toggled = PaintGetClockSpeed(&C.ca) == i;
  }

  {
    RectangleInt r = MainGetTargetRegion();
    PaintSetViewport(&C.ca, r.x, r.y, r.width, r.height);
  }
}

void MainNewFile(Ui* ui) {
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  PaintNewBuffer(&C.ca);
  MainUpdateTitle();
}

void MainUnload() {
  PaintUnload(&C.ca);
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  UnloadRenderTexture(C.img_target_tex);
  C.inited = false;
}

void MainLoadImageFromPath(const char* path) {
  PaintLoadImage(&C.ca, LoadImage(path));
  if (C.fname) {
    free(C.fname);
    C.fname = NULL;
  }
  C.fname = CloneString(path);
  MainUpdateTitle();
}

void MainOpenFileModal(Ui* ui) {
  ModalResult mr = ModalOpenFile(NULL);
  if (mr.ok) {
    MainLoadImageFromPath(mr.path);
    free(mr.path);
  } else if (mr.cancel) {
  } else {
    char txt[500];
    sprintf(txt, "ERROR: %s\n", mr.error_msg);
    MsgAdd(txt, MSG_DURATION);
  }
}

int MainOnSaveClick(Ui* ui, bool saveas) {
  if (C.fname == NULL || saveas) {
    ModalResult mr = ModalSaveFile(NULL, NULL);
    if (mr.ok) {
      if (C.fname) {
        free(C.fname);
        C.fname = NULL;
      }
      C.fname = mr.path;
    } else if (mr.cancel) {
      return 1;
    } else {
      char txt[500];
      sprintf(txt, "ERROR: %s\n", mr.error_msg);
      MsgAdd(txt, MSG_DURATION);
      return -1;
    }
  }

  if (C.fname) {
    Image out = CloneImage(PaintGetEditImage(&C.ca));
    // Before saving, add black pixels back
    ImageAddBlacks(out);
    PaintSetNotDirty(&C.ca);
    if (!ExportImage(out, C.fname)) {
      MsgAdd("ERROR: Could not save image...", MSG_DURATION);
      return -2;
    }
    UnloadImage(out);
    MainUpdateTitle();
    MsgAdd("Image Saved.", MSG_DURATION);
    return 0;
  }
  return 0;
}

void MainAskForSaveAndProceed(Ui* ui, UiCallback next_action) {
  if (!PaintGetIsDirty(&C.ca)) {
    next_action(ui);
  } else {
    DialogOpen(ui, "Do you want to save changes?", next_action);
  }
}

void MainPasteText(const char* txt) {
  Image img = RenderText(txt, C.ca.fg_color);
  PaintPasteImage(&C.ca, img);
}
