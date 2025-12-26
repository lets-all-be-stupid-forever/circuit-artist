#ifndef CA_PROFILER_H
#define CA_PROFILER_H
#include "stdbool.h"

void profiler_init();
void profiler_destroy();
void profiler_reset();
void profiler_tic(const char* name);
void profiler_tic_single(const char* name);
void profiler_tac();
void profiler_tac_single(const char* name);
void profiler_draw();

void miniprof_reset();
void miniprof_time();
void miniprof_nxt();
void miniprof_print(const char* name);

#endif
