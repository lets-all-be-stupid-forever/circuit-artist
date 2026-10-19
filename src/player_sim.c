#include "player_sim.h"

#include "sim.h"

static struct {
  bool sim_loaded;
  bool paused;
  Sim sim;
  HSim hsim;
  double simu_target_steps;
  int pix_toggle;
  int hover_pix; /* Wire pixel under the cursor, -1 if none */
  bool time_open;
  bool sim_compilation_error;
  bool sim_other_error;
} C = {0};

void player_sim_init() {}

bool player_sim_did_compilation_fail() { return C.sim_compilation_error; }
bool player_sim_has_error() { return C.sim_other_error; }

static Status update_sim() {
  Status status = status_ok();
  Sim* pSim = &C.sim;
  float slack_steps = get_simu_slack_steps();

  /* interaction doesn't work when going backward */
  if (C.pix_toggle > -1 && slack_steps >= 0.f) {
    sim_toggle_pixel(pSim, C.pix_toggle);
    hsim_clear_forward_history(&C.hsim);
    C.pix_toggle = -1;
    C.simu_target_steps = C.sim.state.cur_tick + 1;
  }
  slack_steps = get_simu_slack_steps();

  while (status.ok && slack_steps < 0.f) {
    /* Can't go backward */
    if (!hsim_has_prv(&C.hsim)) {
      slack_steps = 0.f;
      C.simu_target_steps = C.sim.state.cur_tick;
      break;
    }
    status = hsim_prv(&C.hsim);
    slack_steps += 1.f;
  }

  while (status.ok && slack_steps >= 1.f) {
    status = hsim_nxt(&C.hsim);
    if (!status.ok) {
      break;
    }
    simu_play_sounds();
    /* Simulation has called Pause() */
    if (C.sim.pause_requested) {
      C.sim.pause_requested = false;
      C.paused = true;
      C.simu_target_steps = C.sim.state.cur_tick;
      // play_sound_click();
      break;
    }
    slack_steps -= 1.f;
  }
  return status;
}

bool player_sim_update() {
  // TODO
}

void player_sim_start(int nl, Image* imgs, RenderTexture2D* layers,
                      LevelAPI* api) {
  /* Status initialization */
  C.sim_loaded = true;
  C.paused = true;
  C.simu_target_steps = 0;
  C.pix_toggle = -1;
  C.hover_pix = -1;
  C.sim_compilation_error false;
  C.sim_other_error = false;

  /* Compilation */
  sim_init(&C.sim, (SimParams){.warmup_cycles = 1,
                               .nl = nl,
                               .img = imgs,
                               .layers = layers,
                               .api = api});
  sim_compile(&C.sim);
  Status s = sim_post_compile(&C.sim);
  if (!s.ok) {
    C.sim_other_error = true;
    fprintf(stderr, "Error in sim \n");
    // TODO destroy hsim
    sim_destroy(&C.sim);
    C.sim = (Sim){0};
    return;
  }
  if (sim_has_errors(&C.sim)) {
    C.sim_compilation_error = true;
    return;
  }
  C.hsim = wrap_sim(&C.sim);
}

void player_sim_stop() {
  hsim_destroy(&C.hsim);
  sim_destroy(&C.sim);
  C.paused = false;
}
