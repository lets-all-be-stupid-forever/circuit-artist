#include <stdio.h>
#include <string.h>

#include "common.h"
#include "msg.h"
#include "paths.h"
#include "ui.h"
#include "win_clipboard.h"

#ifdef WIN32
#define NOGDI   // Prevent GDI definitions that conflict with raylib
#define NOUSER  // Prevent USER definitions we don't need
#include <windows.h>
#undef NOGDI
#undef NOUSER
// Always compile as Windows subsystem to hide console by default
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#ifndef PLATFORM_WEB
#define EMSCRIPTEN_KEEPALIVE
#endif

EMSCRIPTEN_KEEPALIVE bool ca_get_can_paste() {
  return win_clipboard_can_paste();
}

EMSCRIPTEN_KEEPALIVE void ca_msg_add(const char* msg, int dt) {
  return msg_add(msg, dt);
}

EMSCRIPTEN_KEEPALIVE void ca_load_png(const unsigned char* data, int len) {
  win_clipboard_queue_paste(data, len);
}

int main(int argc, char** argv) {
  int show_console = 0;
  register_args(argc, argv);

  // Parse command-line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-console") == 0) {
      show_console = 1;
    }
  }

#ifdef WIN32
  // If -console flag is provided, allocate a console window
  if (show_console) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
  }
#endif

  SetTraceLogLevel(LOG_WARNING);
  paths_init();
  ui_init();
  SetExitKey(0);  // Avoids window closing with escape key
  SetTargetFPS(60);
  ui_run();
  ui_destroy();
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
