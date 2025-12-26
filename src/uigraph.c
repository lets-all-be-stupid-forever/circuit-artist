#include "uigraph.h"

#include "raylib.h"
#include "rlgl.h"

void ui_graph_draw(int x, int y, int nh, int n, int* data) {
  // nh = num in graph
  rlPushMatrix();
  rlTranslatef(x, y, 0);
  rlScalef(3, 3, 1);
  // Ok ill draw a poligon...

  int n0 = n - nh;
  if (n0 < 0) n0 = 0;
  int xx0 = nh - (n - n0);

  int gw = 200;
  int gh = 100;

  int vmax = 0;
  for (int i = n0; i < n; i++) {
    int v = data[i];
    vmax = vmax > v ? vmax : v;
  }

  DrawRectangle(0, 0, gw, gh, BLACK);

  for (int i = n0; i < n - 1; i++) {
    int i0 = i;
    int i1 = i + 1;
    float x0 = xx0 + i0 - n0;
    float x1 = x0 + 1;

    x0 = x0 / nh * gw;
    x1 = x1 / nh * gw;

    float v0 = data[i + 0];
    float v1 = data[i + 1];
    float y0 = v0 / vmax * gh;
    float y1 = v1 / vmax * gh;
    // ta invertido n sei pq..
    y0 = gh - y0;
    y1 = gh - y1;

    DrawLine(x0, y0, x1, y1, WHITE);
  }

  rlPopMatrix();
}
