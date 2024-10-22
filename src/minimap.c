#include "minimap.h"

#include <math.h>
#include <rlgl.h>

#include "colors.h"
#include "hist.h"
#include "img.h"
#include "ui.h"

void CollectSnapshot(Image buf, Image* img) {
  int w = img->width;
  int h = img->height;
  Color* dout = GetPixels(*img);
  Color* din = GetPixels(buf);
  int rw = buf.width;
  int rh = buf.height;

  float ps;
  if (rw > rh) {
    ps = ((float)rw) / w;
  } else {
    ps = ((float)rh) / h;
  }

  int dx = ((ps * w - rw) / 2.0) / ps;
  int dy = ((ps * h - rh) / 2.0) / ps;
  dx = dx < 0 ? 0 : dx;
  dy = dy < 0 ? 0 : dy;
  // Each pixel in the target image has "ds" pixels in the original image.
  for (int y = dy; y < h - dy; y++) {
    for (int x = dx; x < w - dx; x++) {
      float r = 0;
      float g = 0;
      float b = 0;
      int n = 0;
      float fx0 = (x - dx) * ps;
      float fx1 = (x - dx + 1) * ps;
      float fy0 = (y - dy) * ps;
      float fy1 = (y - dy + 1) * ps;

      int x0 = floor(fx0);
      int y0 = floor(fy0);
      int x1 = ceil(fx1) - 1;
      int y1 = ceil(fy1) - 1;

      if (x1 >= rw) x1 = rw - 1;
      if (y1 >= rh) y1 = rh - 1;
      if (x0 >= rw) x0 = rw - 1;
      if (y0 >= rh) y0 = rh - 1;
      if (x0 < 0) x0 = 0;
      if (y0 < 0) y0 = 0;
      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;

      for (int yy = y0; yy <= y1; yy++) {
        for (int xx = x0; xx <= x1; xx++) {
          Color c = din[yy * rw + xx];
          // if (c.a > 0) {
          r = r + c.r;
          g = g + c.g;
          b = b + c.b;
          n = n + 1;
          //}
        }
      }
      r = r / n;
      g = g / n;
      b = b / n;
      // r = r * 0.5;
      // g = g * 0.5;
      // b = b * 0.5;

      dout[y * w + x] = (Color){
          r,
          g,
          b,
          255,
      };

      // x= w/2 --> dw/2
      // x --> (x - dw/2)
    }
  }
}

void MinimapUpdate(Minimap* m, Paint* ca, Ui* ui, int target_w, int target_h) {
  Image ref_pyr = ca->h.buffer[2];
  Image ref_img = ca->h.buffer[0];
  int w = m->hitbox.width - 2 * m->s;
  int h = m->hitbox.height - 2 * m->s;

  bool needs_draw = false;
  double rw = ref_img.width;
  double rh = ref_img.height;
  double ps = -1;  // pixel spacing in image pixels.
  if (rw > rh) {
    ps = ((float)rw) / w;
  } else {
    ps = ((float)rh) / h;
  }

  if (w != m->img.width) {
    m->cnt = 0;
    if (m->img.width != 0) {
      UnloadImage(m->img);
      UnloadTexture(m->tex);
    }
    m->img = GenImageColor(w, h, MAROON);
    m->tex = LoadTextureFromImage(m->img);
    needs_draw = true;
  }

  if (ref_pyr.data != m->ref_pyr.data) {
    m->ref_pyr = ref_pyr;
    needs_draw = true;
  }

  if (m->cnt == 30) {
    needs_draw = true;
    m->cnt = 0;
  }

  if (needs_draw) {
    ImageClearBackground(&m->img, GetLutColor(COLOR_DARKGRAY));
    if (ps < 2) {
      CollectSnapshot(ca->h.buffer[0], &m->img);
    } else if (ps < 4) {
      CollectSnapshot(ca->h.buffer[1], &m->img);
    } else {
      CollectSnapshot(ca->h.buffer[2], &m->img);
    }
    UpdateTexture(m->tex, m->img.data);
  }

  // Camera view size in window pixels
  int sw = target_w;
  int sh = target_h;

  float cx = ca->camera_x;
  float cy = ca->camera_y;
  float cs = ca->camera_s;

  // Camera view size in minimap pixels
  // sw/cs --> How many image pixels are there in the screen (can be more than
  // image) 1 pixel in map = ps pixels in image. pixels in map = (sw/cs)/ps
  int minimap_cw = sw / cs / ps;
  int minimap_ch = sh / cs / ps;

  // Now I need to find the center position of the screen in pixels.
  // that would be -px + sw/2
  int dx = ((rw - ps * w) / 2.0) / ps;
  int dy = ((rh - ps * h) / 2.0) / ps;

  int mcx = (-cx + sw / 2) / cs / ps - dx;
  int mcy = (-cy + sh / 2) / cs / ps - dy;

  minimap_cw = minimap_cw > 3 ? minimap_cw : 3;
  minimap_ch = minimap_ch > 3 ? minimap_ch : 3;

  int x0 = m->hitbox.x;  //' + m->s;
  int y0 = m->hitbox.y;  //'+ m->s;
  m->cam_rect = (Rectangle){
      x0 + mcx - minimap_cw / 2,
      y0 + mcy - minimap_ch / 2,
      minimap_cw,
      minimap_ch,
  };

  // Camera review: cx,cy is the screen pixel position of the origin
  // of the image.
  // cs is the size of each (image) pixel in screen pixels.

  // Now the mouse stuff!
  Vector2 pos = GetMousePosition();
  bool hit = CheckCollisionPointRec(pos, m->hitbox) && ui->hit_count == 0;
  if (hit) {
    ui->hit_count++;
  }

  if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
       IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) &&
      hit) {
    m->pressed = true;
  }

  if ((IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ||
       IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) ||
      !hit) {
    m->pressed = false;
  }
  if (hit) {
    PaintHandleWheelZoom(ca);
  }

  if (m->pressed) {
    float mousex = pos.x - m->hitbox.x - m->s;
    float mousey = pos.y - m->hitbox.y - m->s;
    // essa eh a posicao em pixels.

    // Calcular essas coisas..
    int imgx = (mousex + dx) * ps;
    int imgy = (mousey + dy) * ps;

    // cx=0,0 --> center of screen is sw/2/cs
    // imgx = rw / 2;
    // imgy = rh / 2;
    // esse eh o centro na imagem
    float newcx = -imgx * cs + sw / 2;  //- - sw / 2 / cs;
    float newcy = -imgy * cs + sh / 2;  //- - sh / 2 / cs;

    ca->camera_x = newcx;
    ca->camera_y = newcy;
    PaintEnsureCameraWithinBounds(ca);
  }
  m->cnt++;
}

void MinimapDraw(Minimap* m) {
  int x = m->hitbox.x;
  int y = m->hitbox.y;
  int w = m->hitbox.width;
  int h = m->hitbox.height;
  int s = m->s;

  rlPushMatrix();

  rlPushMatrix();
  rlTranslatef(x, y, 0);
  rlScalef(s, s, 1);

  DrawRectangle(0, 0, w / s, h / s, GetLutColor(COLOR_BTN0));
  DrawRectangle(1, 1, w / s - 2, h / s - 2, BLACK);
  rlPopMatrix();

  // Camera rect
  {
    BeginScissorMode(x + s, y + s, w - 2 * s, h - 2 * s);
    DrawTexture(m->tex, x + s, y + s, WHITE);
    Color k = GetLutColor(COLOR_ORANGE);
    k.a = 250;
    DrawRectangleLinesEx(m->cam_rect, 3, k);
    k.a = 50;
    DrawRectangleRec(m->cam_rect, k);
    EndScissorMode();
  }
  rlPopMatrix();
}
