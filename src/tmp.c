#include "raylib.h"
#include "stdio.h"

int main() {
  printf("Hello World");

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(640 * 2, 320 * 2, "Circuit Artist");

  while (!WindowShouldClose()) {
    BeginDrawing();
    DrawRectangle(0, 0, 2000, 2000, BLACK);
    DrawText("hello world", 20, 20, 20, WHITE);
    EndDrawing();
  }

  return 0;
}

