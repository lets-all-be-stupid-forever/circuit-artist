#ifndef CA_COMPRESSED_STACK_H
#define CA_COMPRESSED_STACK_H

#include "paged_stack.h"

/*
 * Circular stack with cheap support to empty values.
 *
 * Note that it doesnt' track size.
 *
 * It works very similar to a circular PagedStack, with the
 * exception that it will compress empty patches.
 *
 * The way it works is: The layout of memory on paged stack is:
 * <BLOCK0> <BLOCK1> <BLOCK2> etc...
 *
 * The difference
 * NEW_BLOCK = <N> <BLOCK>
 *
 * So , if I have
 *
 * <empty> <empty> <empty> <A> <empty> <B> <C> <empty> <empty> |HEAD
 * I'll just store the 3 blocks and a head value of 2:
 *
 * {
 *  stack: <3, A> <1, B> <0, C>
 *  head: 2
 * }
 *
 * When I push:
 *  if it's an empty block, I just increment the head value.
 *  If it's a non-empty block, I append the head size to the block and include
 *
 * When I pop:
 *   if head is > 0 i just decrement head and return empty block.
 *   if head == 0, i pop the paged stack, and replace head by the value appended
 * to the block.
 */
typedef struct {
  PagedStack s;
  i32 head; /* head trailing blocks */
} PagedCStack;

void paged_cstack_init(PagedCStack* c, u32 page_size);
void paged_cstack_destroy(PagedCStack* c);
void paged_cstack_clear(PagedCStack* c);
int paged_cstack_empty(PagedCStack* c);
Buffer paged_cstack_pop(PagedCStack* c);
void paged_cstack_push(PagedCStack* c, Buffer data);
void paged_cstack_test();

#endif
