#include "ui.h"

#if defined(WITH_OPENMP)
#include <omp.h>
#endif

#ifdef WIN32
#ifndef CA_SHOW_CONSOLE
// This pragma hides the console app terminal in window.
// We might want to make it flexible so people can use it to debug lua scripts.
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#endif

static Ui _ui = {0};

int main() {
  UiLoad(&_ui);
#if defined(WITH_OPENMP)
  // OpenMP is getting all CPU at Windows for some reason...
  // Add limit of 6 threads to reduce its effect until a better solution is
  // found.
  int nt = omp_get_max_threads();
  nt = nt < 6 ? nt : 6;
  omp_set_num_threads(nt);
#endif

  SetExitKey(0);  // Avoids window closing with escape key
  SetTargetFPS(60);
  SetTraceLogLevel(LOG_ERROR);
  while (true) {
    UiUpdateFrame(&_ui);
    if (_ui.should_close) {
      break;
    }
  }
  UiUnload(&_ui);
  CloseWindow();
  return 0;
}
