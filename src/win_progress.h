#ifndef CA_WIN_PROGRESS_H
#define CA_WIN_PROGRESS_H
#include "stdbool.h"

typedef struct {
  void* ctx;
  bool (*update)(void* ctx);
} ProgressCtx;

void win_progress_init();
void win_progress_open(ProgressCtx pc);
void win_progress_update();
void win_progress_draw();
void win_progress_set_text(const char* txt);

#endif
