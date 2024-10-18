#include "tiling.h"

#include "defs.h"
#include <rlgl.h>

void DrawTiledScreen(int s, Texture2D tex, Rectangle src) {
  rlPushMatrix();
  rlScalef(s, s, 1);
  int tilew = src.width;
  int tileh = src.height;
  int screenw = (GetScreenWidth() + s - 1) / s;
  int screenh = (GetScreenHeight() + s - 1) / s;
  int nx = (screenw + tilew - 1) / tilew;
  int ny = (screenh + tileh - 1) / tileh;
  for (int ty = 0; ty < ny; ty++) {
    for (int tx = 0; tx < nx; tx++) {
      int x = tx * tilew;
      int y = ty * tilew;
      // RLAPI void DrawTexturePro(Texture2D texture, Rectangle source,
      // Rectangle dest, Vector2 origin, float rotation, Color tint); // Draw a
      // part of a texture defined by a rectangle with 'pro' parameters
      Rectangle dest = {
          x,
          y,
          tilew,
          tileh,
      };
      Vector2 origin = {0};
      float rotation = 0;
      Color tint = WHITE;
      DrawTexturePro(tex, src, dest, origin, rotation, tint);
    }
  }
  rlPopMatrix();
}

void DrawTiledFrame(int s, FramePatternDesc pd, Rectangle inner_content) {
  // Idea:
  // 1. Draw each side
  // 2. Draw corners on top

  int cx = inner_content.x;
  int cy = inner_content.y;
  int ch = inner_content.height;
  int cw = inner_content.width;
  int th = pd.frame_left.height;
  int tw = pd.frame_left.width;

  // TODO: set Scissor
  BeginScissorMode(cx - s * tw, cy - s * tw, cw + 2 * s * tw, ch + 2 * s * tw);

  int nx = (cw + s * tw - 1) / (s * tw);
  int ny = (ch + s * th - 1) / (s * th);
  rlPushMatrix();
  rlTranslatef(cx, cy, 0);
  rlScalef(s, s, 1);
  rlTranslatef(0, 0, 0);
  for (int ix = 0; ix < nx; ix++) {
    int x = ix * th;
    Rectangle dest = {
        x,
        -tw,
        th,
        tw,
    };
    float rotation = 0;
    Vector2 origin = {0};  //{tw / 2.0, th / .0};
    // Rectangle src = frame;
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_up, dest, origin, rotation, tint);
  }

  // LEft side
  for (int iy = 0; iy < ny; iy++) {
    int y = iy * th;
    Rectangle dest = {
        -tw,
        y,
        tw,
        th,
    };
    float rotation = 0;
    Vector2 origin = {0};  //{tw / 2.0, th / .0};
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_left, dest, origin, rotation, tint);
  }

  // Bottom
  for (int ix = 0; ix < nx; ix++) {
    int x = ix * th;
    Rectangle dest = {
        x,
        ch / s,
        th,
        tw,
    };
    float rotation = 0;
    Vector2 origin = {0};
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_down, dest, origin, rotation, tint);
  }

  // Right side
  for (int iy = 0; iy < ny; iy++) {
    int y = iy * th;
    Rectangle dest = {
        cw / s,
        y,
        tw,
        th,
    };
    float rotation = 0;
    Vector2 origin = {0};  //{tw / 2.0, th / .0};
    Color tint = WHITE;
    DrawTexturePro(pd.tex, pd.frame_right, dest, origin, rotation, tint);
  }

  DrawTexturePro(pd.tex, pd.corner, (Rectangle){-tw, -tw, tw, tw}, (Vector2){0},
                 0, WHITE);
  DrawTexturePro(pd.tex, pd.corner, (Rectangle){cw / s, -tw, tw, tw},
                 (Vector2){0}, 0, WHITE);
  DrawTexturePro(pd.tex, pd.corner, (Rectangle){-tw, ch / s, tw, tw},
                 (Vector2){0}, 0, WHITE);
  DrawTexturePro(pd.tex, pd.corner, (Rectangle){cw / s, ch / s, tw, tw},
                 (Vector2){0}, 0, WHITE);

  rlPopMatrix();
  EndScissorMode();
}

void DrawDefaultTiledScreen(Ui* ui) {
  DrawTiledScreen(ui->scale, ui->sprites, rect_bg_pattern);
}

void DrawDefaultTiledFrame(Ui* ui, Rectangle inner_content) {
  FramePatternDesc pd = {
      .tex = ui->sprites,
      .frame_left = (Rectangle){816, 16, 7, 12},
      .frame_up = (Rectangle){832, 16, 12, 7},
      .frame_right = (Rectangle){848, 16, 7, 12},
      .frame_down = (Rectangle){864, 16, 12, 7},
      .corner = (Rectangle){832, 0, 7, 7},
  };
  DrawTiledFrame(ui->scale, pd, inner_content);
}
