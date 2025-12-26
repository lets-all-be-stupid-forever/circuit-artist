#include "plot.h"

#include "raylib.h"

void plot_graph(Series series, float f) {
  int h = 100;
  int s = 1;
  int n = series.h;
  Color bg = BLACK;
  bg.a = 100;
  DrawRectangle(0, 0, s * n, h, bg);
  double vmax = 0;
  for (int i = 0; i < n; i++) {
    double v = series.data[i];
    vmax = v > vmax ? v : vmax;
  }
  for (int i = 0; i < n - 1; i++) {
    float v0 = f * series.data[(series.first + i) % series.h];
    float v1 = f * series.data[(series.first + i + 1) % series.h];
    int i0 = i * s;
    int i1 = (i + 1) * s;
    int y0 = h * (1.0 - v0 / vmax);
    int y1 = h * (1.0 - v1 / vmax);
    DrawLine(i0, y0, i1, y1, WHITE);
  }
}

void plot_graph2(Series series, int w, int h) {
  int s = 1;
  int n = w;  // series.h;
  Color bg = BLACK;
  bg.a = 100;
  DrawRectangle(0, 0, s * n, h, bg);
  double vmax = 0;
  for (int i = 0; i < n; i++) {
    double v = series.data[i];
    vmax = v > vmax ? v : vmax;
  }
  for (int i = 0; i < n - 1; i++) {
    float v0 = series.data[(series.first + i) % series.h];
    float v1 = series.data[(series.first + i + 1) % series.h];
    int i0 = i * s;
    int i1 = (i + 1) * s;
    int y0 = h * (1.0 - v0 / vmax);
    int y1 = h * (1.0 - v1 / vmax);
    DrawLine(i0, y0, i1, y1, WHITE);
  }
}

void plot_graph_diff(Series series) {
  int h = 100;
  int s = 2;
  int n = series.h;
  Color bg = BLACK;
  bg.a = 100;
  DrawRectangle(0, 0, s * n, h, bg);
  double vmax = 0;
  for (int i = 0; i < n - 1; i++) {
    double v0 = series.data[(series.first + i) % series.h];
    double v1 = series.data[(series.first + i + 1) % series.h];
    double v = v1 - v0;
    vmax = v > vmax ? v : vmax;
  }
  for (int i = 0; i < n - 2; i++) {
    float k0 = series.data[(series.first + i) % series.h];
    float k1 = series.data[(series.first + i + 1) % series.h];
    float k2 = series.data[(series.first + i + 2) % series.h];
    float v0 = k1 - k0;
    float v1 = k2 - k1;
    int i0 = i * s;
    int i1 = (i + 1) * s;
    int y0 = h * (1.0 - v0 / vmax);
    int y1 = h * (1.0 - v1 / vmax);
    DrawLine(i0, y0, i1, y1, WHITE);
  }
}
