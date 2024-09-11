#include "string.h"
#include "ui.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static void WebUpdateDrawFrame(void);  // Update and Draw one frame

static Ui _ui = {0};

int main()
{
  UiLoad(&_ui);
#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(web_update_draw_frame, 0, 1);
#else
  SetExitKey(0);  // Avoids window closing with escape key
  SetTargetFPS(120);
  SetTraceLogLevel(LOG_ERROR);
  while (true) {
    UiUpdateFrame(&_ui);
    if (_ui.should_close) {
      break;
    }
  }
#endif
  UiUnload(&_ui);
  CloseWindow();
  return 0;
}

void WebUpdateDrawFrame()
{
  UiUpdateFrame(&_ui);
}
