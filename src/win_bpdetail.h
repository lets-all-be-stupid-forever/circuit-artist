#ifndef WIN_BPDETAIL_H
#define WIN_BPDETAIL_H

#include "blueprint.h"

typedef enum {
  BPDETAIL_PASTE,
  BPDETAIL_LOADWLEVEL,
} BPDetailMainAction;

void win_bpdetail_init();
void win_bpdetail_open(Blueprint* bp, BPDetailMainAction main_action);
void win_bpdetail_update();
void win_bpdetail_draw();
void win_bpdetail_on_close();

#endif
