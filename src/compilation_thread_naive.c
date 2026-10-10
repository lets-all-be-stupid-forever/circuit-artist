#include "compilation_thread.h"

void stop_compilation() {}
void wait_compilation() {}
bool is_compilation_done() { return true; }
void destroy_comp_thread() {}
bool get_compilation_cancelled(void* ctx) { return false; }
bool is_compilation_cancelled() { return false; }
void init_compilation_thread(Sim* sim) {
  sim->is_cancelled_cb = get_compilation_cancelled;
  sim->is_cancelled_ctx = NULL;
  sim_compile(sim);
}

