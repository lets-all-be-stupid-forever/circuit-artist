#include "ui.h"

#include <lauxlib.h>
#include <lua.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "font.h"
#include "game_registry.h"
#include "level.h"
#include "log.h"
#include "modal.h"
#include "msg.h"
#include "profiler.h"
#include "shaders.h"
#include "stb_ds.h"
#include "stdio.h"
#include "steam.h"
#include "tutorial.h"
#include "ui.h"
#include "utils.h"
#include "wabout.h"
#include "wdialog.h"
#include "win_blueprint.h"
#include "win_campaign.h"
#include "win_level.h"
#include "win_msg.h"
#include "win_stamp.h"
#include "wmain.h"
#include "wnumber.h"
#include "wtext.h"

static struct {
  int scale;           // Global UI pixel scaling.
  Image img_sprites;   // Global UI sprites loaded from ../assets/sprite4.png
  Texture2D sprites;   // Global UI sprites loaded from ../assets/sprite4.png
  WindowEnum* window;  // Active window/screen.
  MouseCursorType cursor;  // Current mouse cursor type.
  bool close_requested;    // If true, will try to close the game.
  bool should_close;       // If True, app will close next frame.
  int hit_count;           // UI hitbox counter for fg/bg
  bool debug;              // Debug flag, for profiling and others.
  bool demo;               // Wether it's demo version.
  double frame_time;       /* Time spend in the frame */
  lua_State* L;            /* Game lua environment (non-sandboxed)*/
  char* lua_error; /* Lua error message: Causes app to go in crash mode */
  GameRegistry* registry;
} C = {0};

static void ui_draw_mouse();
static void ui_close();

void ui_winpush(WindowEnum win) { arrput(C.window, win); }
void ui_winpop() { arrpop(C.window); }

void ui_inc_hit_count() { C.hit_count++; };

Texture2D ui_get_sprites() { return C.sprites; };
Image ui_get_sprites_img() { return C.img_sprites; };

int ui_get_hit_count() { return C.hit_count; };

int ui_get_scale() { return C.scale; }

void ui_crash(const char* error) { C.lua_error = clone_string(error); }

bool ui_is_demo() {
#ifdef DEMO_VERSION
  return true;
#else
  return false;
#endif
}

void ui_init() {
  C.demo = false;
  C.debug = false;
#ifdef WITH_STEAM
  SteamInit();
#endif

  //  For now the ui scale only works at scale=2, there are some hard coded
  //  scale that needs to be fixed (low priority)
  C.scale = 2;

  /* The minimum must be done here so we can display errors */
  int screen_width = 640 * 2;
  int screen_height = 320 * 2;
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(screen_width, screen_height, "Circuit Artist");
  InitAudioDevice();
  C.registry = create_game_registry();
  init_mod(C.registry, "../assets/files");
  load_progress(C.registry);

  load_art_font("../assets/font5x7.png");

  if (C.lua_error) return;
  BeginDrawing();
  // Added this black rectangle here so the screen first appears as a black
  // window rather than a black window with a white square in the center
  // (windows) before window is maximized.
  // TODO: put a propper splash screen eventually?
  DrawRectangle(0, 0, 2000, 2000, BLACK);
  EndDrawing();
  SetWindowState(FLAG_WINDOW_MAXIMIZED);
  C.img_sprites = LoadImage("../assets/sprite4.png");
  C.sprites = LoadTextureFromImage(C.img_sprites);
  HideCursor();
  msg_init();
  win_campaign_init(C.registry);
  win_blueprint_init();
  about_init();
  win_stamp_init();
  profiler_init();
  modal_init();
  tutorial_init(C.registry);
  shaders_init();
  win_level_init(C.registry);
  main_init();
  main_open();
}

void ui_destroy() {
  main_destroy();
  modal_destroy();
#ifdef WITH_STEAM
  SteamShutdown();
#endif
}

static void ui_close() { C.should_close = true; }

static void ui_error_mode() {
  if (WindowShouldClose() || IsKeyPressed(KEY_ESCAPE) ||
      IsKeyPressed(KEY_ENTER)) {
    C.should_close = true;
  }
  BeginDrawing();
  ClearBackground(DARKBLUE);
  rlPushMatrix();
  rlScalef(2, 2, 1);
  draw_text_box(C.lua_error, (Rectangle){20, 20, 400, 0}, WHITE, NULL);
  rlPopMatrix();
  EndDrawing();
}

static void ui_update_debug() {
  if (IsKeyPressed(KEY_F10)) {
    C.debug = !C.debug;
  }
}

void ui_run() {
  double previousTime = GetTime();  // Previous time measure
  double currentTime = 0.0;         // Current time measure
  double updateDrawTime = 0.0;      // Update + Draw time
  double waitTime = 0.0;            // Wait time (if target fps required)
  float deltaTime = 0.0f;
  int targetFPS = 60;
  while (true) {
    PollInputEvents();
    if (ui_get_should_close()) {
      // save_level_progress();
      break;
    }
    if (C.lua_error) {
      ui_error_mode();
    } else {
      ui_update_frame();
    }
    SwapScreenBuffer();
    currentTime = GetTime();
    updateDrawTime = currentTime - previousTime;

    if (targetFPS > 0)  // We want a fixed frame rate
    {
      waitTime = (1.0f / (float)targetFPS) - updateDrawTime;
      if (waitTime > 0.0) {
        WaitTime((float)waitTime);
        currentTime = GetTime();
        deltaTime = (float)(currentTime - previousTime);
      }
    } else {
      deltaTime = (float)updateDrawTime;  // Framerate could be variable
    }
    C.frame_time = deltaTime;
    previousTime = currentTime;
  }
}

void ui_update_frame() {
  // X button in the UI
  if (WindowShouldClose()) {
    C.close_requested = true;
  }

  if (C.close_requested) {
    C.close_requested = false;
    main_ask_for_save_and_proceed(ui_close);
  }

  // Resets the hit count for the frame.
  C.hit_count = 0;
  profiler_reset();

  // *** Update Step ***
  // Assigining here because the variable can change during the update.
  C.cursor = MOUSE_ARROW;
  int update_window = ui_get_window();
  ui_update_debug();
  profiler_tic("GameUpdate");
  // script_update();
  if (update_window == WINDOW_TEXT) text_modal_update();
  if (update_window == WINDOW_NUMBER) number_modal_update();
  if (update_window == WINDOW_STAMP) win_stamp_update();
  if (update_window == WINDOW_MAIN) main_update();
  if (update_window == WINDOW_ABOUT) about_update();
  if (update_window == WINDOW_DIALOG) dialog_update();
  if (update_window == WINDOW_LOG) win_log_update();
  if (update_window == WINDOW_TUTORIAL) tutorial_update();
  if (update_window == WINDOW_LEVEL) win_level_update();
  if (update_window == WINDOW_CAMPAIGN) win_campaign_update();
  if (update_window == WINDOW_BLUEPRINT) win_blueprint_update();
  if (update_window == WINDOW_MSG) win_msg_update();
  profiler_tac();

  // We stop the app here if should_close is flagged.
  if (C.should_close) {
    return;
  }

  // *** Draw Step ***
  BeginDrawing();
  profiler_tic("GameDraw");
  for (int i = 0; i < arrlen(C.window); i++) {
    WindowEnum window = C.window[i];
    if (window == WINDOW_MAIN) main_draw();
    if (window == WINDOW_TEXT) text_modal_draw();
    if (window == WINDOW_NUMBER) number_modal_draw();
    if (window == WINDOW_ABOUT) about_draw();
    if (window == WINDOW_DIALOG) dialog_draw();
    if (window == WINDOW_LOG) win_log_draw();
    if (window == WINDOW_TUTORIAL) tutorial_draw();
    if (window == WINDOW_LEVEL) win_level_draw();
    if (window == WINDOW_CAMPAIGN) win_campaign_draw();
    if (window == WINDOW_BLUEPRINT) win_blueprint_draw();
    if (window == WINDOW_STAMP) win_stamp_draw();
    if (window == WINDOW_MSG) win_msg_draw();
  }
  ui_draw_mouse();
  profiler_tac();
  if (C.debug) {
    profiler_draw();
    //    my_draw_fps(420, 80, C.frame_time);
  }
  EndDrawing();
}

void ui_draw_mouse() {
  // Doesn't draw if cursor is not on screen.
#ifdef __APPLE__
  // Workaround for MACOS:
  // It seems that raylib should start with cursor_on_screen=True but it
  // starts with a false value, the result being the cursor not being visible
  // when the user starts the application.
  static bool use_raylib = false;
  if (IsCursorOnScreen()) {
    use_raylib = true;
  }
  if (use_raylib && !IsCursorOnScreen()) {
    return;
  }
#else
  if (!IsCursorOnScreen()) {
    return;
  }
#endif

  Vector2 pos = GetMousePosition();
  Rectangle source = rect_mouse_arrow;
  switch (C.cursor) {
    case MOUSE_SWITCH: {
      source = rect_mouse_switch;
      break;
    }
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
    case MOUSE_CLOCK: {
      source = rect_mouse_clock;
      break;
    }
  };
  int s = C.scale;
  Rectangle target = {
      .x = pos.x - 16 * s,
      .y = pos.y - 16 * s,
      .width = 32 * s,
      .height = 32 * s,
  };
  DrawTexturePro(C.sprites, source, target, (Vector2){.x = 0, .y = 0}, 0.0f,
                 WHITE);
}

bool ui_get_should_close() { return C.should_close; }

void ui_set_cursor(MouseCursorType cursor) { C.cursor = cursor; }
void ui_set_close_requested() { C.close_requested = true; }
WindowEnum ui_get_window() { return C.window[arrlen(C.window) - 1]; }

double ui_get_frame_time() { return C.frame_time; }

WindowEnum ui_wintop() { return ui_get_window(); }
lua_State* ui_L() { return C.L; }
