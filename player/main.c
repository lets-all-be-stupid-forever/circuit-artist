#include "player.h"
#include "raylib.h"
#include "stdio.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static struct {
  double previous_time;
  double frame_time;
  RenderTexture2D tgt;
} C = {0};

static void frame_control_spin() {
  double target_fps = 60;
  SwapScreenBuffer();
  double current_time = GetTime();
  double update_draw_time = current_time - C.previous_time;
  double wait_time = (1.0 / target_fps) - update_draw_time;
  if (wait_time < 0) wait_time = 0;
  double eps = 0.002;
  double target = wait_time + current_time;
  if (target - current_time > eps) {
    WaitTime(target - current_time - eps);
    current_time = GetTime();
  }
  // busy - wait
  while (current_time < target) {
    current_time = GetTime();
  }
  double delta_time = current_time - C.previous_time;
  C.frame_time = delta_time;
  C.previous_time = current_time;
  PollInputEvents();
}

#if defined(PLATFORM_WEB)
/* On web the browser paces us via requestAnimationFrame, so there is nothing
 * to wait/spin on: just swap, measure the elapsed time and poll input.
 * raylib is built with SUPPORT_CUSTOM_FRAME_CONTROL, so EndDrawing() does
 * none of this for us. */
static void frame_control_web() {
  SwapScreenBuffer();
  double current_time = GetTime();
  double delta_time = current_time - C.previous_time;
  /* rAF stops while the tab is hidden; don't let the sim jump on return. */
  if (delta_time > 0.1) delta_time = 0.1;
  C.frame_time = delta_time;
  C.previous_time = current_time;
  PollInputEvents();
}
#endif

static void _draw_rt_on_screen(RenderTexture2D rt, Vector2 pos) {
  int tw = rt.texture.width;
  int th = rt.texture.height;
  Rectangle source = {
      .x = 0,
      .y = 0,
      .width = tw,
      .height = -th,
  };
  Rectangle target = {
      .x = pos.x,
      .y = pos.y,
      .width = tw,
      .height = th,
  };
  Vector2 v0 = {0};
  DrawTexturePro(rt.texture, source, target, v0, 0.0f, WHITE);
}

static void check_size(RenderTexture2D* t) {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  // int sw = GetRenderWidth();
  // int sh = GetRenderHeight();
  int tw = t->texture.width;
  int th = t->texture.height;
  if (tw != sw || th != sh) {
    UnloadRenderTexture(*t);
    *t = LoadRenderTexture(sw, sh);
  }
}

static void update_draw_frame(void) {
  check_size(&C.tgt);
  player_update(C.frame_time);
  player_render(&C.tgt);
  BeginDrawing();
  ClearBackground(BLACK); /* same as the game behind its sim target */
  _draw_rt_on_screen(C.tgt, (Vector2){0});
  EndDrawing();
#if defined(PLATFORM_WEB)
  frame_control_web();
#else
  frame_control_spin();
#endif
}

int main(int argc, char** argv) {
#if defined(PLATFORM_WEB)
  /* No HIGHDPI on web: raylib's web backend doesn't scale the canvas by
   * devicePixelRatio, but with the flag set its resize callback still divides
   * the reported screen size by it, so GetScreenWidth() would disagree with
   * the actual canvas size and check_size() would build an undersized target.
   */
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
#else
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
#endif
  InitWindow(1000, 800, "player");
  player_init();
  Image img = LoadImage("../assets/example2.png");
  doit(img);
  C.tgt = LoadRenderTexture(800, 800);
  C.previous_time = GetTime();

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(update_draw_frame, 0, 1);
#else
  while (!WindowShouldClose()) {
    update_draw_frame();
  }
  printf("hi\n");
#endif

  return 0;
}
