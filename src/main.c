#include <stdio.h>
#include <string.h>

#include "paths.h"
#include "ui.h"

#ifdef WIN32
#define NOGDI   // Prevent GDI definitions that conflict with raylib
#define NOUSER  // Prevent USER definitions we don't need
#include <windows.h>
#undef NOGDI
#undef NOUSER
// Always compile as Windows subsystem to hide console by default
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int main(int argc, char** argv) {
  int show_console = 0;

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

  paths_init();
  ui_init();
  SetExitKey(0);  // Avoids window closing with escape key
  SetTargetFPS(60);
  SetTraceLogLevel(LOG_WARNING);
  ui_run();
  ui_destroy();
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
