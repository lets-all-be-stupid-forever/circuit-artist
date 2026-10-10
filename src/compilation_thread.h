#ifndef CA_COMPILATION_THREAD_H
#define CA_COMPILATION_THREAD_H

#include "sim.h"
#include "stdbool.h"

void init_compilation_thread(Sim* sim);
void wait_compilation();
void stop_compilation();
bool is_compilation_done();
bool is_compilation_cancelled();
void destroy_comp_thread();

#endif
