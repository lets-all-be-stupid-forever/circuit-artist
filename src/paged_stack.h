#ifndef CA_PAGED_STACK_H
#define CA_PAGED_STACK_H
#include "buffer.h"
#include "common.h"

#define PGD_STK_NUM_PAGES 16

typedef struct {
  u32 n_block;
  u32 head;
  u8 data[];
} PagedStackPage;

typedef struct {
  PagedStackPage* arr_page[PGD_STK_NUM_PAGES];
  int cur_page;   // current page
  u32 page_size;  // max page size in bytes
  int n_block;
} PagedStack;

void paged_stack_init(PagedStack* p, u32 page_size);
void paged_stack_destroy(PagedStack* p);
int paged_stack_get_size(PagedStack* p);
void paged_stack_push(PagedStack* p, Buffer block);
void paged_stack_push_merged(PagedStack* p, Buffer block1, Buffer block2);
void paged_stack_push_raw(PagedStack* p, void* data, u32 size);
void paged_stack_clear(PagedStack* p);
Buffer paged_stack_pop(PagedStack* p);
Buffer paged_stack_peak(PagedStack* p);

#endif
