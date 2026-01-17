#include "hsim.h"

void hsim_init(HSim* h) {
  *h = (HSim){0};
  u32 page_size = 32 * 1024 * 1024;
  int num_pages = 8;
  h->max_patch_size = 16 * 1024 * 1024;
  paged_cstack_init(&h->undo_stack, num_pages, page_size);
  paged_cstack_init(&h->redo_stack, num_pages, page_size);
}

static void hsim_panic_reset_history(HSim* h) {
  paged_cstack_clear(&h->undo_stack);
  paged_cstack_clear(&h->redo_stack);
}

void hsim_clear_forward_history(HSim* h) { paged_cstack_clear(&h->redo_stack); }

void hsim_nxt(HSim* h) {
  Buffer patch = {0};
  if (paged_cstack_empty(&h->redo_stack)) {
    /* Computes new patch */
    patch = h->diff(h->ctx);
  } else {
    /* Uses stored patch */
    patch = paged_cstack_pop(&h->redo_stack);
  }
  h->fwd(h->ctx, patch);
  if (patch.size > h->max_patch_size) {
    hsim_panic_reset_history(h);
  } else {
    paged_cstack_push(&h->undo_stack, patch);
  }
}

bool hsim_has_prv(HSim* h) { return !paged_cstack_empty(&h->undo_stack); }

void hsim_prv(HSim* h) {
  assert(hsim_has_prv(h));
  Buffer patch = paged_cstack_pop(&h->undo_stack);
  paged_cstack_push(&h->redo_stack, patch);
  h->bwd(h->ctx, patch);
}

void hsim_destroy(HSim* h) {
  paged_cstack_destroy(&h->undo_stack);
  paged_cstack_destroy(&h->redo_stack);
}

