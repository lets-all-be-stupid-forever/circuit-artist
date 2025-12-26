#include "pq.h"

#include "assert.h"
#include "stdlib.h"

void pq_init(PQ* p) {
  p->cap = 100;
  p->size = 0;
  p->heap = malloc(p->cap * sizeof(PQElem));
}

void pq_destroy(PQ* p) { free(p->heap); }

static inline void pq_swap(PQElem* a, PQElem* b) {
  PQElem tmp = *a;
  *a = *b;
  *b = tmp;
}

static void heapify_up(PQElem* heap, int index) {
  while (index > 0) {
    int parent = (index - 1) / 2;
    if (heap[parent].priority >= heap[index].priority) {
      break;
    }
    pq_swap(&heap[parent], &heap[index]);
    index = parent;
  }
}

// Heapify down (bubble down) - for max heap
static void heapify_down(PQElem* heap, int size, int index) {
  int maxIndex = index;
  int left = 2 * index + 1;
  int right = 2 * index + 2;

  // Find the element with highest priority among parent and children
  if (left < size && heap[left].priority > heap[maxIndex].priority) {
    maxIndex = left;
  }

  if (right < size && heap[right].priority > heap[maxIndex].priority) {
    maxIndex = right;
  }

  // If parent is not the highest priority, swap and continue heapifying
  if (maxIndex != index) {
    pq_swap(&heap[index], &heap[maxIndex]);
    heapify_down(heap, size, maxIndex);
  }
}

// Insert element with priority
bool pq_push(PQ* p, int src, int dst, float priority) {
  // Add new element at the end
  p->heap[p->size].src = src;
  p->heap[p->size].dst = dst;
  p->heap[p->size].priority = priority;

  // Bubble up to maintain heap property
  heapify_up(p->heap, p->size);
  p->size++;
  if (p->size + 1 >= p->cap) {
    p->cap *= 2;
    p->heap = realloc(p->heap, p->cap * sizeof(PQElem));
  }
  return true;
}

// Remove and return highest priority element
PQElem pq_pop(PQ* p) {
  PQElem empty = {-1, -1};  // Error value

  if (p->size == 0) {
    abort();
    return empty;
  }

  // Store the root (highest priority element)
  PQElem root = p->heap[0];

  // Move last element to root and decrease size
  p->heap[0] = p->heap[p->size - 1];
  p->size--;

  // Bubble down to maintain heap property
  if (p->size > 0) {
    heapify_down(p->heap, p->size, 0);
  }

  return root;
}

// Peek at highest priority element without removing it
PQElem pq_top(PQ* p) {
  PQElem empty = {-1, -1};  // Error value
  if (p->size == 0) {
    abort();
    return empty;
  }
  return p->heap[0];
}
