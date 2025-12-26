#include "buffer.h"

#include "assert.h"
#include "stdlib.h"
#include "string.h"

void buffer_free(Buffer* m) {
  if (m->cap > 0) {
    free(m->data);
    // printf("FREE\n");
    m->data = NULL;
    m->cap = 0;
    m->size = 0;
  }
  assert(m->data == NULL);
  assert(m->size == 0);
}

void buffer_xor(Buffer memA, Buffer memB, Buffer memXor) {
  assert(memA.size == memB.size);
  assert(memA.size == memXor.size);
  for (int i = 0; i < memA.size; i++) {
    memXor.data[i] = memA.data[i] ^ memB.data[i];
  }
}

Buffer buffer_clone(Buffer buffer) {
  if (buffer.size == 0) {
    return (Buffer){0};
  }
  Buffer out = {
      .data = malloc(buffer.size),
      .size = buffer.size,
      .cap = buffer.size,
  };
  // printf("MALLOC_CLONE\n");
  memcpy(out.data, buffer.data, buffer.size);
  return out;
}

Buffer buffer_alloc(u32 size) {
  // printf("MALLOC\n");
  return (Buffer){
      .data = malloc(size),
      .size = size,
      .cap = size,
  };
}

void buffer_memcpy(Buffer dst, Buffer src) {
  assert(dst.size == src.size);
  memcpy(dst.data, src.data, src.size);
}

