#include "paged_stack.h"

#include "assert.h"
#include "stdlib.h"
#include "string.h"

static PagedStackPage* paged_stack_page_forward(PagedStack* p) {
  p->cur_page = (p->cur_page + 1) % PGD_STK_NUM_PAGES;
  int cp = p->cur_page;
  if (!p->arr_page[cp]) {
    p->arr_page[cp] = malloc(p->page_size + sizeof(PagedStackPage));
    p->arr_page[cp]->head = 0;
    p->arr_page[cp]->n_block = 0;
  } else {
    u32 prevBlocks = p->arr_page[cp]->n_block;
    p->arr_page[cp]->head = 0;
    p->arr_page[cp]->n_block = 0;
    p->n_block -= prevBlocks;
  }
  return p->arr_page[cp];
}

void paged_stack_init(PagedStack* p, u32 page_size) {
  assert(page_size > 0);
  p->page_size = page_size;
  p->cur_page = -1;
  p->n_block = 0;
  for (int i = 0; i < PGD_STK_NUM_PAGES; i++) {
    p->arr_page[i] = NULL;
  }
  paged_stack_page_forward(p);
}

void paged_stack_destroy(PagedStack* p) {
  for (int i = 0; i < PGD_STK_NUM_PAGES; i++) {
    if (p->arr_page[i]) {
      free(p->arr_page[i]);
    }
  }
  *p = (PagedStack){0};
}

int paged_stack_get_size(PagedStack* p) { return p->n_block; }

/* Concatenates block1 and block2 before adding them. Useful for storing
 * meta-data on the block */
void paged_stack_push_merged(PagedStack* p, Buffer block1, Buffer block2) {
  u32 size1 = block1.size;
  u32 size2 = block2.size;
  u32 total_size = size1 + size2;
  u32 chunk_size = ((total_size + 3) >> 2) << 2;
  PagedStackPage* page = p->arr_page[p->cur_page];
  u32 new_head = page->head + chunk_size + 4;
  if (new_head > p->page_size) {
    page = paged_stack_page_forward(p);
    new_head = page->head + chunk_size + 4;
  }
  u8* dst = page->data + page->head;
  if (size1 > 0) memcpy(dst, block1.data, size1);
  if (size2 > 0) memcpy(dst + size1, block2.data, size2);
  *((u32*)(dst + chunk_size)) = size1 + size2;
  page->head = new_head;
  page->n_block++;
  p->n_block++;
}

void paged_stack_push(PagedStack* p, Buffer block) {
  paged_stack_push_merged(p, block, (Buffer){0});
}

Buffer paged_stack_pop(PagedStack* p) {
  if (p->n_block == 0) {
    return (Buffer){0};
  }
  PagedStackPage* page = p->arr_page[p->cur_page];
  u32 block_size = *(u32*)(page->data + page->head - 4);
  u32 chunk_size = ((block_size + 3) >> 2) << 2;
  u32 new_head = page->head - sizeof(int) - chunk_size;
  Buffer ret = (Buffer){
      .data = page->data + new_head,
      .size = block_size,
  };
  page->head = new_head;
  page->n_block--;
  p->n_block--;
  if (page->n_block == 0) {
    assert(page->head == 0);
    if (p->n_block > 0) {
      p->cur_page = (p->cur_page + PGD_STK_NUM_PAGES - 1) % PGD_STK_NUM_PAGES;
      assert(p->arr_page[p->cur_page]->n_block > 0);
      assert(p->arr_page[p->cur_page]->n_block <= p->n_block);
      assert(p->arr_page[p->cur_page]->head > 0);
    }
  }
  return ret;
}

Buffer paged_stack_peak(PagedStack* p) {
  if (p->n_block == 0) {
    return (Buffer){0};
  }
  PagedStackPage* page = p->arr_page[p->cur_page];
  u8* head = page->data + page->head;
  u32 block_size = *(u32*)(head - 4);
  u32 chunk_size = ((block_size + 3) >> 2) << 2;
  return (Buffer){
      .data = head - sizeof(int) - chunk_size,
      .size = block_size,
  };
}

void paged_stack_push_raw(PagedStack* p, void* data, u32 size) {
  Buffer mem = {
      .data = data,
      .size = size,
  };
  paged_stack_push(p, mem);
}

void paged_stack_clear(PagedStack* p) {
  p->n_block = 0;
  p->cur_page = 0;
  for (int i = 0; i < PGD_STK_NUM_PAGES; i++) {
    if (p->arr_page[i]) {
      p->arr_page[i]->n_block = 0;
      p->arr_page[i]->head = 0;
    }
  }
}
