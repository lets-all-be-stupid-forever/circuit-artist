#ifndef PQ_H
#define PQ_H
#include "stdbool.h"

// Priority queue implementation (max queue)
typedef struct {
  int src;
  int dst;
  float priority;
} PQElem;

typedef struct {
  PQElem* heap;
  int size;
  int cap;
} PQ;

void pq_init(PQ* p);
void pq_destroy(PQ* p);
bool pq_push(PQ* p, int src, int dst, float priority);
PQElem pq_pop(PQ* p);
PQElem pq_top(PQ* p);

#endif
