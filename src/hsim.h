#ifndef CA_HSIM_H
#define CA_HSIM_H
#include "buffer.h"
#include "paged_cstack.h"

typedef struct {
  void* ctx;                 /*not owned*/
  Buffer (*diff)(void* ctx); /* Buffer owned by ctx */
  void (*fwd)(void* ctx, Buffer patch);
  void (*bwd)(void* ctx, Buffer patch);

  /* Compressed Stacks for undo/redos */
  PagedCStack undo_stack; /* Undo patches (data owned by hsim) */
  PagedCStack redo_stack; /* Redo patches (data owned by hsim) */
  u32 max_patch_size; /* Patches bigger than this will make history collapse. */

} HSim;

void hsim_init(HSim* h);
void hsim_clear_forward_history(HSim* h);
void hsim_nxt(HSim* h);
void hsim_prv(HSim* h);
bool hsim_has_prv(HSim* h);
void hsim_destroy(HSim* h);

#endif
