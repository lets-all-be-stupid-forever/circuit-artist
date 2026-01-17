#ifndef CA_PAGED_STACK_H
#define CA_PAGED_STACK_H
#include "buffer.h"
#include "common.h"

typedef struct {
  u32 n_block;
  u32 head;
  u32 size;
  u8 data[];
} PagedStackPage;

typedef struct {
  int cur_page;           // current page
  u32 default_page_size;  // max page size in bytes
  int n_block;
  int num_pages;
  PagedStackPage* arr_page[];
} PagedStack;

PagedStack* paged_stack_create(int num_pages, u32 page_size);
void paged_stack_destroy(PagedStack* p);
int paged_stack_get_size(PagedStack* p);
void paged_stack_push(PagedStack* p, Buffer block);
void paged_stack_push_merged(PagedStack* p, Buffer block1, Buffer block2);
void paged_stack_push_raw(PagedStack* p, void* data, u32 size);
void paged_stack_clear(PagedStack* p);
Buffer paged_stack_pop(PagedStack* p);
Buffer paged_stack_peak(PagedStack* p);

#endif
