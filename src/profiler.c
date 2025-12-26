#include "profiler.h"

#include "common.h"
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
  } * ravg;
  struct {
    const char* key;
    double value;
  } * tsingle;
  int first;
  int last;
} C = {0};

void profiler_init() {
  if (C.inited) {
    return;
  }
  C.inited = true;
  C.stack_elapsed = malloc(50 * sizeof(double));
  C.stack_cname = malloc(50 * sizeof(const char*));
  C.stack_size = 0;
}

// Resets stats. Needs to be called begin of each frame.
void profiler_reset() {
  if (C.stack_size != 0) {
    abort();
  }
  int n = arrlen(C.elapsed);
  if (n > 0) {
    arrfree(C.elapsed);
    arrfree(C.cname);
  }
}

// Draws profiling stats on screen.
void profiler_draw() {
  int n = arrlen(C.elapsed);
  int yy = 200;
  for (int i = 0; i < shlen(C.tsingle); i++) {
    char txt[200];
    double t = 1000 * C.tsingle[i].value;
    snprintf(txt, sizeof(txt), "%20s: %.1lfms", C.tsingle[i].key, t);
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
    snprintf(txt, sizeof(txt), "%20s: %.1lfms", C.cname[i], t);
    DrawText(txt, 50, yy + 1, 20, BLACK);
    DrawText(txt, 50, yy, 20, LIME);
    yy += 30;
  }
}

// Profiling functions that are called every frame.
// The statistics is the running average.
void profiler_tic(const char* name) {
  C.stack_elapsed[C.stack_size] = GetTime();
  C.stack_cname[C.stack_size] = name;
  C.stack_size++;
}

void profiler_tac() {
  double now = GetTime();
  double elapsed = now - C.stack_elapsed[C.stack_size - 1];
  const char* name = C.stack_cname[C.stack_size - 1];
  arrput(C.elapsed, elapsed);
  arrput(C.cname, name);
  C.stack_size--;
};

// Profiling single-call functions, that are not called every frame.
void profiler_tic_single(const char* name) {
  if (shgeti(C.tsingle, name) != -1) {
    shdel(C.tsingle, name);
  };
  shput(C.tsingle, name, GetTime());
}

void profiler_tac_single(const char* name) {
  double now = GetTime();
  double start = shget(C.tsingle, name);
  shput(C.tsingle, name, now - start);
};

void profiler_destroy() {
  free(C.stack_elapsed);
  free(C.stack_cname);
}

static struct {
  int cnt;
  int imax;
  double last_t;
  double acc[50];
} M = {0};

void miniprof_reset() {
  M.cnt = 0;
  M.imax = 0;
  M.last_t = -1;
  for (int i = 0; i < 50; i++) M.acc[i] = 0;
}

void miniprof_nxt() { M.cnt = 0; }

void miniprof_time() {
  double t = GetTime();
  if (M.cnt > 0) {
    M.acc[M.cnt - 1] += t - M.last_t;
  }
  M.last_t = t;
  M.imax = M.cnt > M.imax ? M.cnt : M.imax;
  M.cnt++;
}

void miniprof_print(const char* name) {
  printf("[%10s] ", name);
  for (int i = 0; i < M.imax; i++) {
    printf("%6.1fms ", 1000 * (M.acc[i]));
  }
  printf("\n");
}

