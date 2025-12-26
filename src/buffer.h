#ifndef CA_buffer_H
#define CA_buffer_H
#include "assert.h"
#include "common.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* Lightweight data to store raw buffers.
 * It can store owned data or a view of a data buffer.
 * */
typedef struct {
  u8* data; /* Buffer data */
  u32 size; /* Number of USED bytes. */
  u32 cap;  /* Allocated bytes. if is 0 data is not owned by the struct. */
} Buffer;

Buffer buffer_clone(Buffer buffer);
Buffer buffer_alloc(u32 size);
void buffer_xor(Buffer srca, Buffer srcb, Buffer dst);
void buffer_memcpy(Buffer dst, Buffer src);
void buffer_free(Buffer* buffer);

// Stack-like behaviour:
static Buffer buffer_push_mem(Buffer* b, Buffer mem);
static Buffer buffer_pop_mem(Buffer* r, u32 bytes);
static void buffer_push_int(Buffer* b, int value);
static int buffer_pop_int(Buffer* r);
static double buffer_pop_double(Buffer* r);
static u8* buffer_push_raw(Buffer* b, u32 bytes, void* ptr);
static void buffer_push_array(Buffer* b, u32 elSize, u32 numel, void* ptr);
static void buffer_reset(Buffer* b);
static void* buffer_pop_array(Buffer* r, u32 elSize, int* numel);
static void* buffer_pop_raw(Buffer* r, u32 bytes);

static inline void* buffer_pop_raw(Buffer* r, u32 bytes) {
  assert(r->size >= bytes);
  r->size -= bytes;
  return r->data + r->size;
}

static inline Buffer buffer_pop_mem(Buffer* r, u32 bytes) {
  return (Buffer){
      .data = buffer_pop_raw(r, bytes),
      .size = bytes,
      .cap = 0,
  };
}

static inline int buffer_pop_int(Buffer* r) {
  return *((int*)buffer_pop_raw(r, sizeof(int)));
}

static inline double buffer_pop_double(Buffer* r) {
  return *((double*)buffer_pop_raw(r, sizeof(double)));
}

static inline void* buffer_pop_array(Buffer* r, u32 elSize, int* numel) {
  *numel = buffer_pop_int(r);
  if (*numel == 0) {
    return NULL;
  }
  return buffer_pop_raw(r, elSize * (*numel));
}

static inline u8* buffer_push_raw(Buffer* b, u32 bytes, void* ptr) {
  u32 prev_size = b->size;
  u32 new_size = prev_size + bytes;
  while (new_size > b->cap) {
    b->cap *= 2;
    b->data = realloc(b->data, b->cap);
    // printf("REALLOC\n");
  }
  b->size = new_size;
  memcpy(b->data + prev_size, ptr, bytes);
  return b->data + prev_size;
}

static inline void buffer_push_int(Buffer* b, int value) {
  buffer_push_raw(b, sizeof(int), &value);
}

static inline void buffer_push_double(Buffer* b, double value) {
  buffer_push_raw(b, sizeof(double), &value);
}

static inline void buffer_push_array(Buffer* b, u32 elSize, u32 numel,
                                     void* ptr) {
  if (numel > 0) {
    buffer_push_raw(b, numel * elSize, ptr);
  }
  buffer_push_int(b, numel);
}

static inline void buffer_reset(Buffer* b) { b->size = 0; }

static inline Buffer buffer_push_mem(Buffer* b, Buffer mem) {
  u8* data = buffer_push_raw(b, mem.size, mem.data);
  return (Buffer){
      .size = mem.size,
      .data = data,
      .cap = 0,
  };
}

#endif
