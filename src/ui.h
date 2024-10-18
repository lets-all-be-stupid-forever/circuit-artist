#ifndef UI_H
#define UI_H
#include <raylib.h>

typedef enum {
  MOUSE_ARROW,
  MOUSE_PEN,
  MOUSE_BUCKET,
  MOUSE_PICKER,
  MOUSE_SELECTION,
  MOUSE_MOVE,
  MOUSE_RESIZE,
  MOUSE_POINTER,
} MouseCursorType;

// Available windows/screens
typedef enum {
  WINDOW_MAIN,
  WINDOW_ABOUT,
  WINDOW_TEXT,
  WINDOW_TUTORIAL,
  WINDOW_LEVELS,
  WINDOW_DIALOG,
} WindowEnum;

// Shared ui state.
typedef struct {
  // Global UI pixel scaling.
  int scale;
  // Global UI sprites loaded from ../assets/sprite4.png
  Texture2D sprites;
  // Active window/screen.
  WindowEnum window;
  // Application's icon.
  Image icon;
  // Current mouse cursor type.
  MouseCursorType cursor;
  // If true, will try to close the game.
  bool close_requested;
  // If True, app will close next frame.
  bool should_close;
  // UI hitbox counter to see which widget is on foreground and which ones are
  // in background (increased with every hit each frame).
  int hit_count;
} Ui;

void UiLoad(Ui* ui);
void UiUpdateFrame(Ui* ui);
void UiUnload(Ui* ui);

#endif
