#include "series.h"

#include "assert.h"
#include "stdlib.h"

void series_init(Series* s, int h) {
  s->data = calloc(h, sizeof(double));
  s->h = h;
  s->first = 0;
}

static double double_xor(double a, double b) {
  union {
    double d;
    uint64_t i;
  } ua = {a}, ub = {b};
  ua.i ^= ub.i;
  return ua.d;
}

void series_destroy(Series* s) { free(s->data); }

void unpack_series(Series* s, Buffer* patch, bool fw) {
  double* diff = buffer_pop_raw(patch, sizeof(double));
  if (fw) {
    s->data[s->first] = double_xor(s->data[s->first], diff[0]);
    s->first = (s->first + 1) % s->h;
  } else {
    int f0 = (s->first + s->h - 1) % s->h;
    s->first = f0;
    s->data[s->first] = double_xor(s->data[s->first], diff[0]);
  }
}

void series_forward(Series* s, double diff) {
  s->data[s->first] = double_xor(s->data[s->first], diff);
  s->first = (s->first + 1) % s->h;
}

void series_backward(Series* s, double diff) {
  int f0 = (s->first + s->h - 1) % s->h;
  s->first = f0;
  s->data[s->first] = double_xor(s->data[s->first], diff);
}

void series_make_push_patch(Series* s, double value, double* patch) {
  *patch = double_xor(value, s->data[s->first]);
}

double series_top(Series* s) { return s->data[(s->first - 1 + s->h) % s->h]; }

