#include "ui.h"

#ifdef WIN32
// This pragma hides the console app terminal in window.
// We might want to make it flexible so people can use it to debug lua scripts.
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include "api.h"
#include "clip_api.h"
#include "colors.h"
#include "font.h"
#include "math.h"
#include "raylib.h"
#include "rlgl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tiling.h"
#include "utils.h"
#include "w_about.h"
#include "w_cpedia.h"
#include "w_dialog.h"
#include "w_levels.h"
#include "w_main.h"
#include "w_text.h"

static void UiDrawMouse(Ui *ui);  // Draws the mouse cursor. It's last thing that is drawn in the screen.
static void UiClose(Ui *ui);      // Callback called when user clicks on the close "X" in the game window.

void UiLoad(Ui *ui)
{
  *ui = (Ui){0};

  // For now the ui scale only works at scale=2, there are some hard coded
  // scale that needs to be fixed (low priority)
  ui->scale = 2;

#ifndef WEB
  int k = 110;
  int screen_width = k * 16 - 2;
  int screen_height = k * 9 - 32;
  SetTraceLogLevel(LOG_NONE);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#else
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  int screen_width = 640 * 2;
  int screen_height = 360 * 2;
#endif
  InitWindow(screen_width, screen_height, "Circuit Artist");
  ui->icon = LoadImage("../assets/icon32.png");
#ifndef WEB
  SetWindowIcon(ui->icon);
#endif
  LoadArtFont("../assets/font5x7.png");
  ui->sprites = LoadTexture("../assets/sprite4.png");
  HideCursor();
  ApiLoad();
  MainOpen(ui);
}

void UiUnload(Ui *ui)
{
  MainUnload();
}

static void UiClose(Ui *ui)
{
  ui->should_close = true;
}

void UiUpdateFrame(Ui *ui)
{
  // X button in the UI
  if (WindowShouldClose()) {
    ui->close_requested = true;
  }

  if (ui->close_requested) {
    ui->close_requested = false;
    MainAskForSaveAndProceed(ui, UiClose);
  }

  // Resets the hit count for the frame.
  ui->hit_count = 0;

  // *** Update Step ***
  // Assigining here because the variable can change during the update.
  ui->cursor = MOUSE_ARROW;
  int update_window = ui->window;
  if (update_window == WINDOW_TEXT) TextModalUpdate(ui);
  if (update_window == WINDOW_MAIN) MainUpdate(ui);
  if (update_window == WINDOW_LEVELS) LevelsUpdate(ui);
  if (update_window == WINDOW_CPEDIA) CpediaUpdate(ui);
  if (update_window == WINDOW_ABOUT) AboutUpdate(ui);
  if (update_window == WINDOW_DIALOG) DialogUpdate(ui);

  // We stop the app here if should_close is flagged.
  if (ui->should_close) {
    return;
  }

  // *** Draw Step ***
  BeginDrawing();
  MainDraw(ui);
  if (ui->window == WINDOW_TEXT) TextModalDraw(ui);
  if (ui->window == WINDOW_ABOUT) AboutDraw(ui);
  if (ui->window == WINDOW_CPEDIA) CpediaDraw(ui);
  if (ui->window == WINDOW_LEVELS) LevelsDraw(ui);
  if (ui->window == WINDOW_DIALOG) DialogDraw(ui);
  UiDrawMouse(ui);
  EndDrawing();
}

void UiDrawMouse(Ui *ui)
{
  // Doesn't draw if cursor is not on screen.
  if (!IsCursorOnScreen()) {
    return;
  }

  Vector2 pos = GetMousePosition();
  Rectangle source = rect_mouse_arrow;
  switch (ui->cursor) {
    case MOUSE_POINTER: {
      source = rect_mouse_pointer;
      break;
    }
    case MOUSE_RESIZE: {
      source = rect_mouse_resize;
      break;
    }
    case MOUSE_MOVE: {
      source = rect_mouse_movecross;
      break;
    }
    case MOUSE_SELECTION: {
      source = rect_mouse_cross2;
      break;
    }
    case MOUSE_ARROW: {
      source = rect_mouse_arrow;
      break;
    }
    case MOUSE_PEN: {
      source = rect_mouse_pen_alone;
      break;
    }
    case MOUSE_PICKER: {
      source = rect_mouse_picker;
      break;
    }
    case MOUSE_BUCKET: {
      source = rect_mouse_bucket;
      break;
    }
  };
  int s = ui->scale;
  Rectangle target = {
      .x = pos.x - 16 * s,
      .y = pos.y - 16 * s,
      .width = 32 * s,
      .height = 32 * s,
  };
  DrawTexturePro(ui->sprites, source, target, (Vector2){.x = 0, .y = 0}, 0.0f, WHITE);
}

