#include "compilation_thread.h"

/* tinycthread.h includes windows.h on MSVC. raylib.h is already included above
 * (via sim.h) and windows.h's GDI/USER headers redeclare ShowCursor, LoadImage,
 * DrawText, CloseWindow and Rectangle. Neither is needed for threads. */
#ifdef _WIN32
#define NOGDI
#define NOUSER
#define NOMINMAX
#endif
#include "tinycthread.h"

static struct {
  thrd_t thr;
  mtx_t mut; /* protects done/cancelled */
  bool done;
  bool cancelled;
  bool joined;
} C = {0};

void stop_compilation() {
  mtx_lock(&C.mut);
  C.cancelled = true;
  mtx_unlock(&C.mut);
}

void wait_compilation() {
  if (C.joined) return;
  thrd_join(C.thr, NULL);
  C.joined = true;
}

bool get_compilation_cancelled(void* ctx) {
  mtx_lock(&C.mut);
  bool ret = C.cancelled;
  mtx_unlock(&C.mut);
  return ret;
}

bool is_compilation_done() {
  mtx_lock(&C.mut);
  bool ret = C.done;
  mtx_unlock(&C.mut);
  return ret;
}

void destroy_comp_thread() {
  wait_compilation();
  mtx_destroy(&C.mut);
  C.done = false;
  C.cancelled = false;
  C.joined = false;
}

static int sim_compile_thread(void* ctx) {
  Sim* sim = ctx;
  int r = sim_compile(sim);
  mtx_lock(&C.mut);
  C.done = true;
  mtx_unlock(&C.mut);
  return r;
}

bool is_compilation_cancelled() { return get_compilation_cancelled(NULL); }

void init_compilation_thread(Sim* sim) {
  sim->is_cancelled_cb = get_compilation_cancelled;
  sim->is_cancelled_ctx = NULL;
  mtx_init(&C.mut, mtx_plain);
  int ok = thrd_create(&C.thr, sim_compile_thread, sim);
  if (ok != thrd_success) {
    fprintf(stderr, "Failed to create thread.");
    abort();
  }
}
