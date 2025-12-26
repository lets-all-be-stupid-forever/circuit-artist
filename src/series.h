#ifndef CA_SERIES_H
#define CA_SERIES_H

#include "buffer.h"
#include "stdbool.h"

typedef struct {
  double* data;
  int h;
  int first;
} Series;

void series_init(Series* s, int h);
void series_destroy(Series* s);
void series_forward(Series* s, double diff);
void series_backward(Series* s, double diff);
void series_make_push_patch(Series* s, double value, double* patch);
double series_top(Series* s);

void unpack_series(Series* s, Buffer* patch, bool fw);

#endif
