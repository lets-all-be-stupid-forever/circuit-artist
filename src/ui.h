#ifndef CA_UI_H
#define CA_UI_H
#include <lua.h>

#include "raylib.h"
#include "stdbool.h"

typedef enum {
  MOUSE_ARROW,
  MOUSE_PEN,
  MOUSE_BUCKET,
  MOUSE_PICKER,
  MOUSE_SELECTION,
  MOUSE_SWITCH,
  MOUSE_MOVE,
  MOUSE_RESIZE,
  MOUSE_POINTER,
  MOUSE_CLOCK,
} MouseCursorType;

// Available windows/screens
typedef enum {
  WINDOW_MAIN,
  WINDOW_ABOUT,
  WINDOW_TEXT,
  WINDOW_NUMBER,
  WINDOW_MSG,
  WINDOW_DIALOG,
  WINDOW_TUTORIAL,
  WINDOW_STAMP,
  WINDOW_LOG,
  WINDOW_LEVEL,
  WINDOW_CAMPAIGN,
  WINDOW_BLUEPRINT,
} WindowEnum;

void ui_init();
void ui_update_frame();
void ui_destroy();
bool ui_get_should_close();
int ui_get_scale();
void ui_winpush(WindowEnum newWindow);
void ui_winpop();
WindowEnum ui_wintop();
WindowEnum ui_get_window();
Texture2D ui_get_sprites();
Image ui_get_sprites_img();
void ui_inc_hit_count();
int ui_get_hit_count();
void ui_set_cursor(MouseCursorType cursor);
void ui_set_close_requested();
void ui_run();
void ui_crash(const char* err);
double ui_get_frame_time();
void ui_handle_lua_error(lua_State* L);

#endif
