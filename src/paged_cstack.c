#include "paged_cstack.h"

void paged_cstack_init(PagedCStack* c, int num_pages, u32 page_size) {
  c->s = paged_stack_create(num_pages, page_size);
  c->head = 0;
}

void paged_cstack_clear(PagedCStack* c) {
  paged_stack_clear(c->s);
  c->head = 0;
}

int paged_cstack_empty(PagedCStack* c) {
  return c->head == 0 && paged_stack_get_size(c->s) == 0;
}

Buffer paged_cstack_pop(PagedCStack* c) {
  if (c->head > 0) {
    c->head--;
    return (Buffer){0};
  }
  assert(paged_stack_get_size(c->s) > 0);
  Buffer ret = paged_stack_pop(c->s);
  c->head = *((i32*)ret.data);
  ret.data += sizeof(i32);
  ret.size -= sizeof(i32);
  return ret;
}

void paged_cstack_push(PagedCStack* c, Buffer data) {
  if (data.size == 0) {
    c->head++;
    return;
  }
  Buffer meta = {.data = (u8*)&c->head, .size = sizeof(i32)};
  paged_stack_push_merged(c->s, meta, data);
  c->head = 0;
}

void paged_cstack_destroy(PagedCStack* c) { paged_stack_destroy(c->s); }

void paged_cstack_test() {
  PagedCStack c = {0};
  paged_cstack_init(&c, 8, 4 * 16);
  int k = 0;
  Buffer empty = {0};
  Buffer tmp = {0};
  Buffer crap = buffer_alloc(24);
  for (int i = 0; i < crap.size; i++) {
    crap.data[i] = 123 + i;
  }

  assert(paged_cstack_empty(&c));
  paged_cstack_push(&c, empty);
  assert(!paged_cstack_empty(&c));
  tmp = paged_cstack_pop(&c);
  assert(paged_cstack_empty(&c));
  assert(tmp.size == 0);
  paged_cstack_push(&c, empty);
  paged_cstack_push(&c, crap);
  paged_cstack_push(&c, crap);
  paged_cstack_push(&c, empty);
  tmp = paged_cstack_pop(&c);
  assert(tmp.size == 0);
  tmp = paged_cstack_pop(&c);
  tmp = paged_cstack_pop(&c);
  assert(tmp.size == crap.size);
  for (int i = 0; i < crap.size; i++) {
    assert(tmp.data[i] == crap.data[i]);
  }
  tmp = paged_cstack_pop(&c);
  assert(paged_cstack_empty(&c));
  assert(tmp.size == 0);

  buffer_free(&crap);
}
