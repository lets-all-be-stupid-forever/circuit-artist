#include "raylib.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"

static struct {
  bool inited;
  double* stack_elapsed;
  const char** stack_cname;

  double* elapsed;
  const char** cname;
  int stack_size;

  struct {
    const char* key;
    double value;
  }* ravg;

  struct {
    const char* key;
    double value;
  }* tsingle;

  int first;
  int last;
} C = {0};

void ProfilerInit() {
  if (C.inited) {
    return;
  }
  C.inited = true;
  C.stack_elapsed = malloc(50 * sizeof(double));
  C.stack_cname = malloc(50 * sizeof(const char*));
  C.stack_size = 0;
}

void ProfilerReset() {
  if (C.stack_size != 0) {
    abort();
  }
  int n = arrlen(C.elapsed);
  if (n > 0) {
    arrfree(C.elapsed);
    arrfree(C.cname);
  }
}

void ProfilerDraw() {
  int n = arrlen(C.elapsed);
  int yy = 200;
  for (int i = 0; i < shlen(C.tsingle); i++) {
    char txt[200];
    double t = 1000 * C.tsingle[i].value;
    sprintf(txt, "%20s: %.1lfms", C.tsingle[i].key, t);
    DrawText(txt, 50, yy + 1, 20, BLACK);
    DrawText(txt, 50, yy, 20, LIME);
    yy += 30;
  }

  for (int i = 0; i < n; i++) {
    char txt[200];
    double t = 1000 * C.elapsed[i];
    int k = shgeti(C.ravg, C.cname[i]);
    if (k == -1) {
      shput(C.ravg, C.cname[i], t);
    }
    double f = 0.9;
    t = f * shget(C.ravg, C.cname[i]) + (1 - f) * t;
    shput(C.ravg, C.cname[i], t);
    sprintf(txt, "%20s: %.1lfms", C.cname[i], t);
    DrawText(txt, 50, yy + 1, 20, BLACK);
    DrawText(txt, 50, yy, 20, LIME);
    yy += 30;
  }
}

void ProfilerTic(const char* name) {
  ProfilerInit();
  C.stack_elapsed[C.stack_size] = GetTime();
  C.stack_cname[C.stack_size] = name;
  C.stack_size++;
}

void ProfilerTac() {
  double now = GetTime();
  double elapsed = now - C.stack_elapsed[C.stack_size - 1];
  const char* name = C.stack_cname[C.stack_size - 1];
  arrput(C.elapsed, elapsed);
  arrput(C.cname, name);
  C.stack_size--;
};

void ProfilerTicSingle(const char* name) {
  if (shgeti(C.tsingle, name) != -1) {
    shdel(C.tsingle, name);
  };
  shput(C.tsingle, name, GetTime());
}

void ProfilerTacSingle(const char* name) {
  double now = GetTime();
  double start = shget(C.tsingle, name);
  shput(C.tsingle, name, now - start);
};

