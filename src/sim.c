#include "sim.h"

#include <stb_ds.h>

#include "assert.h"
#include "colors.h"
#include "font.h"
#include "graph.h"
#include "img.h"
#include "log.h"
#include "math.h"
#include "msg.h"
#include "paged_cstack.h"
#include "pixel_graph.h"
#include "plot.h"
#include "profiler.h"
#include "rlgl.h"
#include "shaders.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"

enum {
  PATCH_LEVL = (1 << 0),
  PATCH_QPOP = (1 << 1),
  PATCH_SKCT = (1 << 2),
  PATCH_PULS = (1 << 3), /* Wire Pulses */
  PATCH_SCHD = (1 << 4), /* Schedules items */
  PATCH_NAND = (1 << 5),
  PATCH_MAXP = (1 << 6), /* Max pulse */
  PATCH_E_TICK = (1 << 7),
  PATCH_ECLK = (1 << 8),
  PATCH_MAXT = (1 << 9),
  PATCH_CYCL = (1 << 10),
  PATCH_ERROR = (1 << 11),
  PATCH_DONE = (1 << 12),
  PATCH_WTOSH = (1 << 13),
};

#define getnwire(s) (s->wg.nwire)
#define isskt(s) (s == PIN_IMG2LUA)
#define isdrv(s) (s == PIN_LUA2IMG)

static inline int mini(int a, int b) { return a < b ? a : b; }
static inline int maxi(int a, int b) { return a > b ? a : b; }

static int sim_get_num_nands(Sim* sim) { return arrlen(sim->pg.nands); }

static void print_graph(Graph* g) {
  for (int i = 0; i < g->n; i++) {
    int me = g->max_edges;
    int ne = g->ecount[i];
    for (int e = 0; e < ne; e++) {
      int kk = g->edges[i * me + e].e;
      int ww = g->edges[i * me + e].w;
      printf("%d -- %d --> %d\n", i, ww, kk);
    }
  }
}

static float clamp(float a, float b, float x) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}

static float smoothstep(float a, float b, float x) {
  float t = clamp(0, 1, (x - a) / (b - a));
  return t * t * (3 - 2 * t);
}

static void dispatch_lone_wires(Sim* sim) {
  // need to find all lone wires
  int* w2drv = sim->wg.wire_to_drv;
  int initialWireValue = 0;
  for (int wire = 0; wire < getnwire(sim); wire++) {
    int drv = w2drv[wire];
    if (drv == -1) {
      patch_builder_dispatch(&sim->patch_builder, &sim->state, wire,
                             initialWireValue);
    }
  }
}

static void print_bits(int64_t num) {
  for (int i = 63; i >= 0; i--) {
    printf("%d", (int)((num >> i) & 1));
    if (i % 8 == 0) printf(" ");  // Optional: space every 8 bits
  }
}

PinComm sim_port_read(Sim* sim, int iport) {
  int nw = arrlen(sim->api->pg[iport].pins);
  assert(nw <= 64);
  i64 b = 0;
  i64 f = 0;
  int iskt = sim->pg.pgoff[iport];
  for (int iw = 0; iw < nw; iw++) {
    /* Reading Socket */
    i64 vv = sim->state.skt_values[iskt++];
    b |= (vv & 1LL) << iw;
    f |= ((vv & 2LL) >> 1) << iw;
  }
  return (PinComm){b, f};
}

void sim_port_write(Sim* sim, int iport, PinComm pc) {
  int nw = arrlen(sim->api->pg[iport].pins);
  assert(nw <= 64);
  assert(isdrv(sim->api->pg[iport].type));
  int idrv = sim->pg.pgoff[iport];
  for (int iw = 0; iw < nw; iw++) {
    /* Reading Socket */
    int bb = (pc.b >> iw) & 1LL;
    int ff = (pc.f >> iw) & 1LL;
    int nxtv = (ff == 1) ? 2 : bb;
    int c = sim->wg.drv_to_wire[idrv++];
    if (c >= 0) {
      patch_builder_dispatch(&sim->patch_builder, &sim->state, c, nxtv);
    }
  }
}

static Status sim_update_level(Sim* sim) {
  Status s = status_ok();
  if (sim->api && sim->api->update) {
    Buffer buffer = {0};
    s = sim->api->update(sim->api->u, &buffer);
    if (s.ok) {
      sim->patch_builder.level_updated = true;
      sim->patch_builder.level_patch = buffer;
    }
  }
  return s;
}

void sim_register_nands(Sim* sim, Image img) {
  int w = sim->w;
  NandLoc* nands = sim->pg.nands;
  Color* colors = img.data;
  int nn = arrlen(nands);
  for (int i = 0; i < nn; i++) {
    int idx0 = nands[i].s1;
    int idx1 = nands[i].s2;
    int idx2 = nands[i].d;
    int x0 = idx0 % w;
    int y0 = idx0 / w;
    int x1 = idx1 % w;
    int y1 = idx1 / w;
    int x2 = idx2 % w;
    int y2 = idx2 / w;
    int rot = -1;
    int x = mini(x0, mini(x1, x2));
    int y = mini(y0, mini(y1, y2));
    if (x0 == x1) {
      if (x2 > x1) {
        // right
        rot = 0;
        x2 -= 2;
        x0 += 2;
        x1 += 2;
      } else {
        // left
        rot = 2;
        x2 += 2;
        x0 -= 2;
        x1 -= 2;
      }
    } else {
      if (y2 > y1) {
        // down
        rot = 1;
        y2 -= 2;
        y0 += 2;
        y1 += 2;
      } else {
        // Up
        rot = 3;
        y2 += 2;
        y0 -= 2;
        y1 -= 2;
      }
    }
    Color c1 = colors[y0 * w + x0];
    Color c2 = colors[y1 * w + x1];
    Color c3 = colors[y2 * w + x2];
    // this is not enough because of the api driver ...
    int bugged = sim->wg.drv_status[i] | sim->wg.skt_status[2 * i + 0] |
                 sim->wg.skt_status[2 * i + 1];
    Vector2 p1 = {x0, y0};
    Vector2 p2 = {x1, y1};
    Vector2 p3 = {x2, y2};
    renderv2_addnand(sim->rv2, p1, p2, p3, c1, c2, c3);
    int idx = y2 * w + x2;
    arrput(sim->nidx, ((NandDesc){idx, rot, c1, c2, c3}));
  }

  /* Registering drivers that come from levels */
  int offdrv = sim_get_num_nands(sim);
  int ndrv = arrlen(sim->pg.drv);
  for (int i = 0; i < ndrv; i++) {
    int idx = sim->pg.drv[i];
    /* Using rotation of 0 for drivers */
    arrput(sim->nidx, ((NandDesc){idx, 0, 0, 0, 0}));
  }
}

/*
 * Will put all wires to 0 and bad wires with an error flag.
 */
static void sim_add_errors_to_state(Sim* sim) {
  WirePulse* pulses = sim->state.pulses;
  int tmod = sim->state.tick_mod;
  int tgap = tmod / sim->state.tick_slots;
  int t0 = (tmod - tgap * 3) % tmod;
  // Initialize all pulse array elements (pulse_size may be > num_wires)
  for (int i = 0; i < sim->state.pulse_size; i++) {
    pulses[i] = pulse_pack2(0, 0, t0);
  }
  int ndrv = arrlen(sim->pg.drv);
  for (int i = 0; i < ndrv; i++) {
    int wire = sim->wg.drv_to_wire[i];
    if (wire < 0) continue;
    if (sim->wg.drv_status[i] & STATUS_CONFLICT) {
      pulses[wire] = pulse_pack2(2, 2, 0);
    } else if (sim->wg.drv_status[i] & STATUS_TOOSLOW) {
      pulses[wire] = pulse_pack2(3, 3, 0);
    }
  }
}

static void collect_bugged_pixels(Sim* sim) {
  int w = sim->w;
  int ndrv = arrlen(sim->pg.drv);
  int nskt = arrlen(sim->pg.skt);
  for (int i = 0; i < ndrv; i++) {
    if (sim->wg.drv_status[i]) {
      int idx = sim->pg.drv[i];
      if (idx >= 0) {
        int x = idx % w;
        int y = idx / w;
        renderv2_add_err_pixel(sim->rv2, x, y);
      }
    }
  }
  for (int i = 0; i < nskt; i++) {
    if (sim->wg.skt_status[i]) {
      int idx = sim->pg.skt[i];
      if (idx == -1) continue;
      int x = idx % w;
      int y = idx / w;
      renderv2_add_err_pixel(sim->rv2, x, y);
    }
  }
  /* Collect bad NANDs for error mode rendering */
  int nnand = sim_get_num_nands(sim);
  for (int i = 0; i < nnand; i++) {
    int bugged = sim->wg.drv_status[i] | sim->wg.skt_status[2 * i + 0] |
                 sim->wg.skt_status[2 * i + 1];
    if (bugged) {
      renderv2_add_bad_nand(sim->rv2, i);
    }
  }
}

static Texture create_pulse_texture(int num_wires) {
  int w = 4096;
  int h = (num_wires + w - 1) / w;
  Image tmp = GenImageColor(w, h, GREEN);
  Texture tex = LoadTextureFromImage(tmp);
  UnloadImage(tmp);
  return tex;
}

static void make_nand_lut(int* nand_lut) {
  // 9 values to setup
  // Off = 0
  // On = 1
  // X = 2 (undefined)
  // NAND(X,0) --> 1
  // NAND(X,1) --> X
  // NAND(X,X) --> X
  // NAND(0,0) --> 1
  // NAND(1,0) --> 1
  // NAND(1,1) --> 0
  int a0 = 0;
  int a1 = 1;
  int ax = 2;
  int b0 = a0 << 2;
  int b1 = a1 << 2;
  int bx = ax << 2;

  nand_lut[b0 + a0] = a1;
  nand_lut[b0 + a1] = a1;
  nand_lut[b0 + ax] = a1;
  nand_lut[b1 + a0] = a1;
  nand_lut[b1 + a1] = a0;
  nand_lut[b1 + ax] = ax;
  nand_lut[bx + a0] = a1;
  nand_lut[bx + a1] = ax;
  nand_lut[bx + ax] = ax;
}

static void patch_builder_init(PatchBuilder* patch_builder, double vdd,
                               double c_gate) {
  patch_builder->level_patch.size = 0;
  patch_builder->level_patch.data = NULL;
  patch_builder->power_tick_series_patch = 0;
  patch_builder->energy_per_cycle_series_patch = 0;
  patch_builder->ticks_per_cycle_series_patch = 0;
  patch_builder->out_patch = buffer_alloc(100);
  patch_builder->out_patch.size = 0;
  patch_builder->max_tick_patch = 0;
  patch_builder->acc_nrj_patch = 0;
  patch_builder->power_patch = 0;
  patch_builder->total_energy_patch = 0;

  for (int i = 0; i < NRJ_BINS; i++) {
    patch_builder->nrj_patch[i] = 0;
  }

  patch_builder->arr_nand_state = NULL;
  patch_builder->arr_skt_diff = NULL;
  patch_builder->arr_pulse_diff = NULL;
  patch_builder->arr_queue_popped = NULL;
  patch_builder->arr_wire_to_shift = NULL;
  patch_builder->arr_schedule_item = NULL;

  patch_builder->arr_nand_state_len = 0;
  patch_builder->arr_skt_diff_len = 0;
  patch_builder->arr_pulse_diff_len = 0;
  patch_builder->arr_queue_popped_len = 0;
  patch_builder->arr_schedule_item_len = 0;

  patch_builder->flags = 0;
  patch_builder->max_pulse_time_diff = 0;
  patch_builder->cycle = false;
  patch_builder->level_updated = false;

  patch_builder->k_vdd_square = vdd * vdd;
  patch_builder->k_gate_energy = 0.5 * c_gate * vdd * vdd;

  make_nand_lut(patch_builder->nand_lut);
}

static void sim_state_init(SimState* state, int num_wire, int pulse_size,
                           int num_drivers, int num_sockets) {
  state->max_pulse_time = 0;
  state->pulse_size = pulse_size;
  state->total_energy = 0.0;
  state->cur_tick = 0;
  state->max_tick = 1;
  /* The visualization uses
   *
   * 2 bits --> "from state"
   * 2 bits --> "to state"
   * 6 bits --> non-integer distance
   * 22 bits --> actual pulse time
   *
   * I'll then use 22 bits for mod.
   * 22bit = 4M
   * It means it "goes around" every 4M ticks.
   *
   * */
#if 1
  state->tick_mod = (1 << 20); /* 1 << 20 = 1M */
  state->tick_slots = 1 << 5;  /* each slot = 1 << 15 = 32k */
#else
  state->tick_mod = (1 << 17); /* 1 << 17 = 128k */
  state->tick_slots = 1 << 3;  /* each slot = 1 << 14 = 16k */
#endif

  // state->ld = ldef;
  series_init(&state->power_tick_series, 600);
  series_init(&state->energy_per_cycle_series, 100);
  series_init(&state->ticks_per_cycle_series, 100);

  state->active_count = 0;
  state->skt_values = malloc(num_sockets * sizeof(int));
  for (int skt = 0; skt < num_sockets; skt++) {
    state->skt_values[skt] = S_BIT_UNDEFINED;
  }
  state->nand_states = calloc(num_drivers, sizeof(NandState));
  state->pulses = malloc(state->pulse_size * sizeof(WirePulse));
  int tmod = state->tick_mod;
  int tgap = tmod / state->tick_slots;
  int t0 = (tmod - tgap * 3) % tmod;
  // Initialize ALL pulse array elements, not just num_wire
  // pulse_size may be larger than num_wire due to texture alignment
  for (int i = 0; i < state->pulse_size; i++) {
    state->pulses[i] = pulse_pack((UnpackedPulse){
        .before_value = S_BIT_UNDEFINED,
        .after_value = S_BIT_UNDEFINED,
        .pulse_tick = t0,
    });
  }
  event_queue_init(&state->ev_queue, SIM_MAX_QUEUE_DELAY);

  /* idea is to have Power = E*exp(- 2 t/T))
   *
   * each bin is a time frame: bin[k] = T=2^(k+1)
   * pow[k] := pow[k] * f_k (timestep of t=1)
   * and pow_tot = sum_k pow[k]
   * or E_tot := E_tot + pow_tot
   * */
  for (int i = 0; i < NRJ_BINS; i++) {
    /* I want the energy to almost fade after T ticks
     * E -> E(1-f)  -> E(1-f)^2
     * I want to have (1-f)^T = 1%, so it drops to 1% after T ticks.
     * then,
     * f = 1-0.01^(1/T)
     *
     */
    double T = pow(2, i + 1);
    double f = pow(0.01, 1.0 / T);
    state->pow_decay[i] = 1.0 - f;
  }
}

static void sim_init_state(Sim* sim) {
  int nw = getnwire(sim);
  sim->pulse_tex = create_pulse_texture(nw);
  int pulse_size = sim->pulse_tex.width * sim->pulse_tex.height;
  int ndrv = arrlen(sim->pg.drv);
  int nskt = arrlen(sim->pg.skt);
  sim_state_init(&sim->state, nw, pulse_size, ndrv, nskt);
  sim->state.sim = sim;
  if (!sim_has_errors(sim)) {
    patch_builder_init(&sim->patch_builder, sim->dist_spec.vdd,
                       sim->dist_spec.c_gate);
    // sim->patch_builder.fg = &sim->fg;
    sim->patch_builder.dg = &sim->dg;
    sim->patch_builder.wg = &sim->wg;
    sim->patch_builder.pg = &sim->pg;
    dispatch_lone_wires(sim);
  } else {
    sim_add_errors_to_state(sim);
  }
}

void init_spec(DistSpec* d) {
  /*
   * P pixels should propagate in 1 tick
   * Formula for straight line is:
   *  (r*l)*(l*c)*0.5 = 0.5*r*c*l^2
   *
   *  So, I want to:
   *
   *  0.5*r*c*l^2 = 1 for l=P
   *
   *  Only the product rc matter.
   *  assuming c=1:
   *  0.5*r*P^2 = 1 = 2/(P^2) ==>
   *
   * The r/w and c/w both change per layer, in different ways.
   * resistance can be 5-10x lower, but capacitance 2-3x lower.
   * But i'll model as both chaning the same, and the difference is modelled in
   * the plen_lut.
   * */
  float P = 10.0;

  float R_corr = 0.05 * 2.0 / (P * P);

  /* average length for basis of nand activation delay.
   * The idea is that when wire length is "avg_length",
   * the delay time of nand and the wire propagation is
   * the same, but when it's smaller, the NAND
   * activation wins and when its bigger, the wire
   * propagation is slower (not taking into account the
   * intrinsic fixed delay time of nand (fixed_gate_delay)) */
  float ld = 40;

  double vdd = 0.5;
  // d->plen_lut[0] = 1.0;
  // d->plen_lut[1] = 0.5;
  // d->plen_lut[2] = 0.25;
  // d->plen_lut[3] = 0.2;
  d->c_per_w[0] = 1.0;
  d->c_per_w[1] = 1.5;
  d->c_per_w[2] = 2.0;

  d->r_per_w[0] = R_corr * 1.0 / d->c_per_w[0];
  d->r_per_w[1] = R_corr * 0.5 / d->c_per_w[1];
  d->r_per_w[2] = R_corr * 0.25 / d->c_per_w[2];

  d->fixed_gate_delay = 2;

  /* C_internal ~ 4 * C_input
   * Tryint to emulate a FO4 delay:
   * The internal delay should be approx the delay
   * of having to connect to 4 sockets.
   * */
  d->l_socket = 0.25 * ld;
  d->c_gate = ld * d->c_per_w[0];
  d->r_gate = ld * d->r_per_w[0];
  d->vdd = vdd;
  d->lone_pulse_energy = 0.5 * d->c_gate * vdd * vdd;
}

static void sim_add_driver_status(Sim* sim, int wire, int status) {
  int idrv = sim->wg.wire_to_drv[wire];
  sim->wg.drv_status[idrv] |= status;
  sim->wg.global_error_flags |= status;
  if (status != STATUS_OK) {
    sim->wg.has_errors = true;
  }
}

/* Enforces a limit on the wire delay so we don't have issues with the cyclic
 * event queue. */
static void sim_check_max_delay(Sim* sim, int max_delay) {
  int nw = getnwire(sim);
  for (int i = 0; i < nw; i++) {
    int d = sim->dg.wprop[i].max_delay;
    if (d > max_delay) {
      sim_add_driver_status(sim, i, STATUS_TOOSLOW);
    }
  }
}

/*
 * The PinGroup is the pin API for external component(s).
 */
Status sim_init(Sim* sim, int nl, Image* img, LevelAPI* api,
                RenderTexture2D* layers) {
  *sim = (Sim){0};
  bool debug = false;
  sim->base_tps = 240;
  Status status = status_ok();
  sim->api = api;
  double start = GetTime();
  sim->poked = false;
  init_spec(&sim->dist_spec);
  sim->nl = nl;
  sim->pinbuf = malloc(arrlen(api->pg) * sizeof(PinComm));
  sim->w = img[0].width;
  sim->h = img[0].height;
  pixel_graph_init(&sim->pg, sim->dist_spec, nl, img, api->pg, debug);
  wire_graph_init(&sim->wg, sim->nl, sim->w, sim->h, &sim->pg, debug);
  sim->num_wire = getnwire(sim);
  sim->rv2 = renderv2_create(sim->w, sim->h, sim->num_wire, sim->nl, layers);
  // sim->rv2->bg_color = (Color){21, 11, 3, 255};
  sim->rv2->bg_color = BLACK;

  int nskt = arrlen(sim->pg.skt);
  dist_graph_init(&sim->dg, sim->dist_spec, sim->w, sim->h, sim->nl, &sim->pg.g,
                  sim->wg.wire_to_drv, sim->pg.drv, sim->wg.wire_to_skt,
                  sim->wg.wire_to_skt_off, sim->pg.skt, nskt, getnwire(sim),
                  sim->wg.comp, sim->pg.ori, sim->rv2, debug);
  int max_delay = SIM_MAX_WIRE_DELAY;
  sim_check_max_delay(sim, max_delay);

  sim->dirty_mask_size = (sim->num_wire + 31) / 32;
  sim->pulse_dirty_mask = calloc(sim->dirty_mask_size, sizeof(uint32_t));
  profiler_tic_single("renderer_init");
  miniprof_print("renderer_init");
  // printf("t0, t1, t2 = %.1fms %.1fms %.1fms\n", 1000 * acc1, 1000 * acc2,
  //        1000 * acc3);
  sim_register_nands(sim, img[0]);
  profiler_tic_single("init2");
  bool has_errors = sim_has_errors(sim);
  if (has_errors) {
    sim->rv2->error_mode = 1;
    collect_bugged_pixels(sim);
  }
  sim_init_state(sim);
  if (sim->api && !has_errors) {
    if (status.ok && sim->api->start) {
      status = sim->api->start(sim->api->u, sim);
    }
    if (status.ok) status = sim_update_level(sim);
    if (!status.ok) {
      return status;
    }
  }

  int tickgap = sim->state.tick_mod / sim->state.tick_slots;
  renderv2_prepare(sim->rv2, sim->state.tick_mod, tickgap);

  if (sim_has_errors(sim)) {
    if (sim->wg.global_error_flags & STATUS_CONFLICT) {
      msg_add(
          "Multiple NAND gates connected to same wire. Max 1 per wire "
          "allowed.",
          -1);
    }
    if (sim->wg.global_error_flags & STATUS_DISCONNECTED) {
      msg_add("Some NANDs are missing wire connections.", -1);
    }
    if (sim->wg.global_error_flags & STATUS_TOOSLOW) {
      msg_add("Some wires are too long/slow. Try breaking them down.", -1);
    }
  }
  profiler_tac_single("init2");
  printf("num_nands=%d\n", sim_get_num_nands(sim));
  printf("parsing=%dms\n", (int)((GetTime() - start) * 1000));
  return status;
}

void sim_destroy(Sim* sim) {
  if (!sim_has_errors(sim)) {
    patch_builder_destroy(&sim->patch_builder);
  }
  pixel_graph_destroy(&sim->pg);
  wire_graph_destroy(&sim->wg);
  dist_graph_destroy(&sim->dg);
  UnloadTexture(sim->pulse_tex);
  free(sim->pulse_dirty_mask);
  renderv2_free(sim->rv2);
  if (sim->light_ema) texdel(sim->light_ema);
  if (sim->circ_ema) texdel(sim->circ_ema);
  sim_state_destroy(&sim->state);
  free(sim->pinbuf);
  free(sim->switch_delay);
  free(sim->switch_energy);
  arrfree(sim->nidx);
  arrfree(sim->ui_events);
  *sim = (Sim){0};
  msg_clear_permanent();
}

Tex* sim_render_energy(Sim* sim, int tw, int th) {
  Tex* out = texnew(tw, th);
  BeginTextureMode(out->rt);
  plot_graph2(sim->state.power_tick_series, tw, th);
  EndTextureMode();
  return out;
}

void sim_find_nearest_pixel(Sim* sim, int tol, v2 fpix, int* pix) {
  assert(tol >= 0);
  int w = sim->w;
  int h = sim->h;
  int nl = sim->nl;
  int x = (int)floorf(fpix.x);
  int y = (int)floorf(fpix.y);
  int x0 = maxi(x - tol, 0);
  int x1 = mini(x + tol, w - 1);
  int y0 = maxi(y - tol, 0);
  int y1 = mini(y + tol, h - 1);

  float dmin = 10000;
  int xmin = -1;
  int ymin = -1;
  int lmin = -1;
  for (int l = nl - 1; l >= 0; l--) {
    int* wire_img = sim->wg.wmap[l];
    for (int yy = y0; yy <= y1; yy++) {
      for (int xx = x0; xx <= x1; xx++) {
        // center of pixel
        float cx = xx + 0.5f;
        float cy = yy + 0.5f;
        float dx = cx - fpix.x;
        float dy = cy - fpix.y;
        float sdist = dx * dx + dy * dy;
        int idx = yy * w + xx;
        int c = wire_img[idx];
        if (c != -1 && sdist < dmin) {
          dmin = sdist;
          xmin = xx;
          lmin = l;
          ymin = yy;
        }
      }
    }
  }
  if (xmin != -1) {
    *pix = (lmin * h + ymin) * w + xmin;
  } else {
    *pix = -1;
  }
}

float sim_get_pixel_dist(Sim* sim, int pix) {
  int w = sim->w;
  int h = sim->h;
  int s = w * h;
  int l = pix / s;
  int idx = pix - l * s;
  float* dist_img = sim->dg.distmap[l];
  float v = dist_img[idx];
  return v < 0 ? -v : v;
}

void sim_toggle_pixel(Sim* sim, int pix) {
  int w = sim->w;
  int h = sim->h;
  int s = w * h;
  int l = pix / s;
  int idx = pix - l * s;
  assert(l < sim->nl);
  int* wire_img = sim->wg.wmap[l];
  int c = wire_img[idx];
  int v = pulse_unpack_vafter(sim->state.pulses[c]);
  int nextV = 2;
  if (v == 0) nextV = 1;
  if (v == 1) nextV = 0;
  if (v == 2) nextV = 0;
  // Don't want to simulate background component
  if (c >= 0) {
    patch_builder_dispatch(&sim->patch_builder, &sim->state, c, nextV);
    sim->poked = true;
  }
}

#define ALPHA_OFF 50
#define ALPHA_ON 255

static Color state2color(Color c, int v) {
  Color undefined = MAGENTA;
  Color bugged = RED;
  if (v == S_BIT_ZERO || v == S_BIT_ONE) {
    int color_alpha = v == S_BIT_ZERO ? ALPHA_OFF : ALPHA_ON;
    c.a = color_alpha;
    return c;
  } else {
    if (v == S_BIT_UNDEFINED) {
      c = undefined;
    } else if (v == S_BIT_BUGGED) {
      c = bugged;
    }
  }
  return c;
}

bool sim_is_idle(Sim* sim) {
  return (!sim_state_has_work(&sim->state) &&
          patch_builder_empty(&sim->patch_builder));
}

bool sim_has_errors(Sim* sim) { return sim->wg.has_errors; }

static inline float float_xor(float a, float b) {
  union {
    float f;
    uint32_t i;
  } ua = {a}, ub = {b}, ur;
  ur.i = ua.i ^ ub.i;
  return ur.f;
}

static inline double double_xor(double a, double b) {
  union {
    double d;
    uint64_t i;
  } ua = {a}, ub = {b};
  ua.i ^= ub.i;
  return ua.d;
}

void patch_builder_update_nrj(PatchBuilder* builder, SimState* state) {
  double power = 0;
  for (int i = 0; i < NRJ_BINS; i++) {
    /* nrj_patch --> Energy added this cycle. */
    assert(builder->nrj_patch[i] >= 0);
    assert(state->energy_t[i] >= 0);
    float f = state->pow_decay[i];
    float cur_energy = builder->nrj_patch[i] + state->energy_t[i];
    float cur_power = f * cur_energy;
    power += cur_power;
    float new_energy = (1.0 - f) * cur_energy;
    /* When nrj_patch=0 -->
     * cur_power = power[t]
     * new_power =
     *
     * power[t+1] := decay[t] * power[t]
     * this_power := sum_t power[t]
     *
     * After this loop: nrj_patch <-- next power value
     * */
    // if (new_power < 1e-5) new_power = 0;
    builder->nrj_patch[i] = float_xor(new_energy, state->energy_t[i]);
  }
  builder->power_patch = double_xor(power, builder->power_patch);
  series_make_push_patch(&state->power_tick_series, power,
                         &builder->power_tick_series_patch);
  double acc_nrj = power + state->acc_nrj;
  double new_total_energy = state->total_energy + power;
  builder->total_energy_patch =
      double_xor(state->total_energy, new_total_energy);

  if (builder->cycle) {
    /* acc_nrj resets to 0 */
    builder->flags |= PATCH_ECLK;
    /* Resets acc_energy in every cycle */
    builder->acc_nrj_patch = double_xor(0, state->acc_nrj);
    /* Actual accumulated energy goes to series */
    series_make_push_patch(&state->energy_per_cycle_series, acc_nrj,
                           &builder->energy_per_cycle_series_patch);

    int tc = state->cur_tick + 1;
    series_make_push_patch(&state->ticks_per_cycle_series, tc,
                           &builder->ticks_per_cycle_series_patch);

    int mt = tc - series_top(&state->ticks_per_cycle_series);
    /* Doesnt' compute stat for first simulation cycle because it does a lot
     * of initialization.  */
    if ((mt > state->max_tick) && state->cycle >= 1) {
      builder->flags |= PATCH_MAXT;
      builder->max_tick_patch = mt ^ state->max_tick;
    }
  } else {
    builder->acc_nrj_patch = double_xor(acc_nrj, state->acc_nrj);
  }
}

static void sim_reset_ui_events(Sim* sim) { arrsetlen(sim->ui_events, 0); }

static void sim_pulse_adjust(Sim* sim) {
  SimState* state = &sim->state;
  PatchBuilder* builder = &sim->patch_builder;
  int mod = state->tick_mod;
  int mod_tick = state->cur_tick % mod;
  int n = state->tick_slots;
  int m = mod / n;
  /* Only activates when multiple of m */
  if (mod_tick % m != 0) {
    return;
  }
  // printf("Shifting pulses! t=%d (%d)\n", mod_tick, state->cur_tick);
  int nwire = getnwire(sim);
  int t1 = (mod_tick + m) % mod;
  int t2 = t1 + m - 1;
  int delta = m * (n - 2);
  for (int i = 0; i < nwire; i++) {
    int prev_pulse = state->pulses[i];
    UnpackedPulse up = pulse_unpack(prev_pulse);
    int t = up.pulse_tick;
    if (t >= t1 && t <= t2) {
      // printf("schedule_shift: w=%d t0=%d\n", i, t);
      arrput(builder->arr_wire_to_shift, i);
    }
  }
}

static Status sim_diff(void* ctx, Buffer* patch) {
  Sim* sim = ctx;
  Status s = status_ok();
  sim_reset_ui_events(sim);
  SimState* state = &sim->state;
  PatchBuilder* builder = &sim->patch_builder;
  sim_pulse_adjust(sim);
  patch_builder_update_nandstate(sim, builder, state);
  patch_builder_handle_socket_events(builder, state);
  patch_builder_remove_duplicated_nand(builder);
  bool with_level = sim_is_idle(sim);
  builder->cycle = with_level && !sim->state.done && !sim->state.error;
  /* Only updates/moves cycle forward when level is not complete/stopped. */
  int interval = sim->update_interval;
  if (builder->cycle && interval == 0) {
    s = sim_update_level(sim);
  }
  if (interval > 0) {
    int t = state->cur_tick;
    if (t % interval == 0 && t != 0) {
      s = sim_update_level(sim);
    }
  }
  if (s.ok) {
    patch_builder_update_nrj(builder, state);
    *patch = patch_builder_commit(&sim->patch_builder, state);
  }
  return s;
}

static Status unpack_level(SimState* state, Buffer* patch, bool fw) {
  int size = buffer_pop_int(patch);
  Buffer buf = buffer_pop_mem(patch, size);
  LevelAPI* api = state->sim->api;
  Status s = status_ok();
  if (fw && api->fw) s = api->fw(api->u, buf);
  if (!fw && api->bw) s = api->bw(api->u, buf);
  return s;
}

static void unpack_nrj(SimState* state, Buffer* patch, bool fw) {
  double power_xor = buffer_pop_double(patch);
  double acc_nrj_xor = buffer_pop_double(patch);
  float* arr = buffer_pop_raw(patch, NRJ_BINS * sizeof(float));
  for (int i = 0; i < NRJ_BINS; i++) {
    state->energy_t[i] = float_xor(arr[i], state->energy_t[i]);
  }
  state->power = double_xor(state->power, power_xor);
  state->acc_nrj = double_xor(state->acc_nrj, acc_nrj_xor);
}

static inline int shift_pulse(int p, int delta, int mod) {
  UnpackedPulse up = pulse_unpack(p);
  up.pulse_tick = (up.pulse_tick + delta + mod) % mod;

  return pulse_pack(up);
}

static void unpack_wire_to_shift(SimState* state, Buffer* patch, bool fw) {
  int len;
  int* wire_to_shift = buffer_pop_array(patch, sizeof(int), &len);
  int mod = state->tick_mod;
  int n = state->tick_slots;
  int m = mod / n;
  int delta = m * (n - 2);
  uint32_t* mask = state->sim->pulse_dirty_mask;
  if (fw) {
    int t = (state->cur_tick + 1) % mod;
    for (int i = 0; i < len; i++) {
      // I only shift if it hasnt been updated
      int w = wire_to_shift[i];
      int tp = pulse_unpack_tick(state->pulses[w]);
      if (tp != t) {
        int t0 = pulse_unpack_tick(state->pulses[w]);
        state->pulses[w] = shift_pulse(state->pulses[w], delta, mod);
        int t1 = pulse_unpack_tick(state->pulses[w]);
        // printf("shifted_pulse: w=%d t0=%d t1=%d\n", w, t0, t1);
        // Mark wire as dirty for texture update
        int off = w / 32;
        int r = w & 31;
        mask[off] = mask[off] | (1u << r);
      }
    }
  } else {
    int t = (state->cur_tick) % mod;
    for (int i = 0; i < len; i++) {
      int w = wire_to_shift[i];
      int tp = pulse_unpack_tick(state->pulses[w]);
      if (tp != t) {
        state->pulses[w] = shift_pulse(state->pulses[w], -delta, mod);
        // Mark wire as dirty for texture update
        int off = w / 32;
        int r = w & 31;
        mask[off] = mask[off] | (1u << r);
      }
    }
  }
}

static void unpack_skt(SimState* state, Buffer* patch) {
  int len;
  SocketValueDiff* items =
      buffer_pop_array(patch, sizeof(SocketValueDiff), &len);
  for (int i = 0; i < len; i++) {
    int iskt = items[i].skt;
    int xor_value = items[i].xor_value;
    state->skt_values[iskt] = state->skt_values[iskt] ^ xor_value;
  }
}

static void unpack_event(SimState* state, Buffer* patch, bool fw) {
  int len;
  SocketEvent* items = buffer_pop_array(patch, sizeof(SocketEvent), &len);
  for (int i = 0; i < len; i++) {
    event_queue_schedule(&state->ev_queue, 0, items[i].socket, items[i].value);
  }
}

static void unpack_flags(int flags, SimState* state, bool fw) {
  if (fw) {
    if (flags & PATCH_CYCL) state->cycle++;
    if (flags & PATCH_ERROR) state->error = true;
    if (flags & PATCH_DONE) state->done = true;
  } else {
    if (flags & PATCH_CYCL) state->cycle--;
    if (flags & PATCH_ERROR) state->error = false;
    if (flags & PATCH_DONE) state->done = false;
  }
}

static inline void axori(int s, int n, void* pa, void* pb, void* pout) {
  int* a = pa;
  int* b = pb;
  int* out = pout;
  int nn = n * s / sizeof(int);
  for (int i = 0; i < nn; i++) {
    out[i] = a[i] ^ b[i];
  }
}

static void unpack_nand_state(SimState* state, Buffer* patch) {
  int n1 = buffer_pop_int(patch);
  Buffer b = buffer_pop_mem(patch, n1 * sizeof(NandState));
  NandState* a0 = state->nand_states;
  NandState* a1 = (NandState*)b.data;
  axori(sizeof(NandState), n1, a0, a1, a0);
  int xn = buffer_pop_int(patch);
  state->active_count ^= xn;
}

static void unpack_schedule(SimState* state, Buffer* patch, bool fw) {
  int len;
  ScheduleItem* items = buffer_pop_array(patch, sizeof(ScheduleItem), &len);
  for (int i = 0; i < len; i++) {
    int dt = items[i].dt_ticks;
    if (fw) {
      int skt = items[i].ev.socket;
      int newValue = items[i].ev.value;
      event_queue_schedule(&state->ev_queue, dt, skt, newValue);
    } else {
      event_queue_unschedule(&state->ev_queue, dt);
    }
  }
}

static void unpack_pulse(SimState* state, Buffer* patch, bool fw) {
  int len;
  PulseDiff* items = buffer_pop_array(patch, sizeof(PulseDiff), &len);
  int mod = state->tick_mod;
  int t = state->cur_tick % mod;
  uint32_t* mask = state->sim->pulse_dirty_mask;
  if (fw) {
    int nxt = (t + 1) % mod;
    for (int j = 0; j < len; j++) {
      int i = len - j - 1;
      int w = items[i].wire;
      int xor_v = items[i].xor_value;
      int new_pulse = state->pulses[w] ^ xor_v;
      int pt = pulse_unpack_tick(new_pulse);
      if (pt == nxt) {
        int off = w / 32;
        int r = w & 31;
        mask[off] = mask[off] | (1u << r);
        state->pulses[w] ^= xor_v;
      }
    }
  } else {
    for (int j = 0; j < len; j++) {
      int i = len - j - 1;
      int w = items[i].wire;
      int xor_v = items[i].xor_value;
      int new_pulse = state->pulses[w] ^ xor_v;
      int pt = pulse_unpack_tick(new_pulse);
      if (pt != t) {
        int off = w / 32;
        int r = w & 31;
        mask[off] = mask[off] | (1u << r);
        state->pulses[w] ^= xor_v;
      }
    }
  }
}

static Status patch_unpack(SimState* state, Buffer patch, bool fw) {
  Status s = status_ok();
  int flags = buffer_pop_int(&patch);
  unpack_flags(flags, state, fw);
  if (flags & PATCH_LEVL) {
    s = unpack_level(state, &patch, fw);
    if (!s.ok) return s;
  }
  if (!fw) event_queue_step_backward(&state->ev_queue);
  state->total_energy =
      double_xor(state->total_energy, buffer_pop_double(&patch));
  unpack_series(&state->power_tick_series, &patch, fw);
  if (flags & PATCH_ECLK) {
    unpack_series(&state->energy_per_cycle_series, &patch, fw);
    unpack_series(&state->ticks_per_cycle_series, &patch, fw);
  }
  if (flags & PATCH_MAXT) state->max_tick ^= buffer_pop_int(&patch);
  if (flags & PATCH_NAND) unpack_nand_state(state, &patch);
  if (flags & PATCH_SCHD) unpack_schedule(state, &patch, fw);
  if (flags & PATCH_WTOSH) unpack_wire_to_shift(state, &patch, fw);
  if (flags & PATCH_PULS) unpack_pulse(state, &patch, fw);
  if (flags & PATCH_SKCT) unpack_skt(state, &patch);
  if (flags & PATCH_QPOP) unpack_event(state, &patch, fw);
  unpack_nrj(state, &patch, fw);
  if (flags & PATCH_MAXP) state->max_pulse_time ^= buffer_pop_int(&patch);
  if (fw) event_queue_step_forward(&state->ev_queue);
  state->cur_tick += fw ? 1 : -1;
  return s;
}

static Status sim_fwd(void* ctx, Buffer patch) {
  Sim* sim = ctx;
  return patch_unpack(&sim->state, patch, true);
}

static Status sim_bwd(void* ctx, Buffer patch) {
  Sim* sim = ctx;
  return patch_unpack(&sim->state, patch, false);
}

HSim wrap_sim(Sim* sim) {
  HSim h = {0};
  hsim_init(&h);
  h.ctx = sim;
  h.diff = sim_diff;
  h.fwd = sim_fwd;
  h.bwd = sim_bwd;
  return h;
}

void sim_state_destroy(SimState* state) {
  series_destroy(&state->power_tick_series);
  series_destroy(&state->energy_per_cycle_series);
  series_destroy(&state->ticks_per_cycle_series);
  free(state->skt_values);
  free(state->nand_states);
  free(state->pulses);
  event_queue_destroy(&state->ev_queue);
}

bool sim_state_has_work(SimState* state) {
  // active_count accounts for active NAND gates.
  // pending_events account for scheduled events (ie sockets waiting to
  // activate) pulsetime is the time after which the last wire stops
  // propagating.
  return state->active_count > 0 || state->ev_queue.pending_events > 0 ||
         state->cur_tick <= (state->max_pulse_time);
}

int sim_state_get_max_tick(SimState* state) { return state->max_tick; }

void patch_builder_reset(PatchBuilder* pb) {
  assert(pb->level_patch.size == 0);
  pb->flags = 0;
  pb->cycle = false;
  pb->level_updated = false;
  pb->max_pulse_time_diff = 0;
  pb->power_tick_series_patch = 0;
  pb->energy_per_cycle_series_patch = 0;
  pb->out_patch.size = 0;

  pb->acc_nrj_patch = 0;
  pb->power_patch = 0;
  for (int i = 0; i < NRJ_BINS; i++) {
    pb->nrj_patch[i] = 0;
  }
  arrsetlen(pb->arr_nand_state, 0);
  arrsetlen(pb->arr_pulse_diff, 0);
  arrsetlen(pb->arr_skt_diff, 0);
  arrsetlen(pb->arr_queue_popped, 0);
  arrsetlen(pb->arr_wire_to_shift, 0);
  arrsetlen(pb->arr_schedule_item, 0);
}

void patch_builder_destroy(PatchBuilder* patch_builder) {
  buffer_free(&patch_builder->out_patch);
  arrfree(patch_builder->arr_nand_state);
  arrfree(patch_builder->arr_pulse_diff);
  arrfree(patch_builder->arr_skt_diff);
  arrfree(patch_builder->arr_queue_popped);
  arrfree(patch_builder->arr_wire_to_shift);
  arrfree(patch_builder->arr_schedule_item);
}

bool patch_builder_empty(PatchBuilder* pb) {
  bool empty = true;
  empty = empty && arrlen(pb->arr_nand_state) == 0;
  empty = empty && arrlen(pb->arr_pulse_diff) == 0;
  empty = empty && arrlen(pb->arr_skt_diff) == 0;
  empty = empty && arrlen(pb->arr_queue_popped) == 0;
  empty = empty && arrlen(pb->arr_wire_to_shift) == 0;
  empty = empty && arrlen(pb->arr_schedule_item) == 0;
  empty = empty && (!pb->level_updated);
  empty = empty && (!pb->cycle);
  return empty;
}

#define PACK_MEM(pb, aname, flag)           \
  if (pb->aname.size > 0) {                 \
    pb->flags |= flag;                      \
    buffer_push_mem(patch, pb->aname);      \
    buffer_push_int(patch, pb->aname.size); \
    pb->aname.size = 0;                     \
  }

#define UNPACK_MEM(aname, flag)                    \
  if (builder->flags & flag) {                     \
    int size = buffer_pop_int(&patch);             \
    builder->aname = buffer_pop_mem(&patch, size); \
  } else {                                         \
    builder->aname.size = 0;                       \
  }

#define PACK_ARRAY(pb, aname, flag)                                \
  if (arrlen(pb->aname) > 0) {                                     \
    pb->flags |= flag;                                             \
    buffer_push_array(patch, sizeof(((PatchBuilder*)0)->aname[0]), \
                      arrlen(pb->aname), pb->aname);               \
  }

#define UNPACK_ARRAY(aname, flag)                                             \
  if (builder->flags & flag) {                                                \
    builder->aname = buffer_pop_array(                                        \
        &patch, sizeof(((PatchBuilder*)0)->aname[0]), &builder->aname##_len); \
  } else {                                                                    \
    builder->aname = NULL;                                                    \
    builder->aname##_len = 0;                                                 \
  }

#define UNPACK_INT(aname, flag)              \
  if (builder->flags & flag) {               \
    builder->aname = buffer_pop_int(&patch); \
  } else {                                   \
    builder->aname = 0;                      \
  }

void pack_nand_state(PatchBuilder* pb, SimState* state, Buffer* patch) {
  NandState* a0 = state->nand_states;
  NandState* a1 = pb->arr_nand_state;
  int n0 = state->active_count;
  int n1 = arrlen(a1);
  if (n1 == n0 && n1 == 0) {
    return;
  }
  pb->flags |= PATCH_NAND;
  axori(sizeof(NandState), n1, a0, a1, a1);
  Buffer buf = {
      .data = (u8*)a1,
      .size = n1 * sizeof(NandState),
  };
  int xn = n1 ^ n0;
  buffer_push_int(patch, xn);
  buffer_push_mem(patch, buf);
  buffer_push_int(patch, n1);
}

Buffer patch_builder_commit(PatchBuilder* pb, SimState* state) {
  Buffer* patch = &pb->out_patch;
  if (pb->cycle) pb->flags |= PATCH_CYCL;
  if (pb->level_updated) pb->flags |= PATCH_LEVL;
  if (pb->max_pulse_time_diff > 0) {
    pb->flags |= PATCH_MAXP;
    int d = state->max_pulse_time ^
            (state->max_pulse_time + pb->max_pulse_time_diff);
    buffer_push_int(patch, d);
  }
  buffer_push_raw(patch, NRJ_BINS * sizeof(float), pb->nrj_patch);
  buffer_push_double(patch, pb->acc_nrj_patch);
  buffer_push_double(patch, pb->power_patch);
  PACK_ARRAY(pb, arr_queue_popped, PATCH_QPOP);
  PACK_ARRAY(pb, arr_skt_diff, PATCH_SKCT);
  PACK_ARRAY(pb, arr_pulse_diff, PATCH_PULS);
  PACK_ARRAY(pb, arr_wire_to_shift, PATCH_WTOSH);
  PACK_ARRAY(pb, arr_schedule_item, PATCH_SCHD);
  pack_nand_state(pb, state, patch);
  if (pb->flags & PATCH_MAXT) {
    buffer_push_int(patch, pb->max_tick_patch);
  }
  if (pb->flags & PATCH_ECLK) {
    buffer_push_double(patch, pb->ticks_per_cycle_series_patch);
    buffer_push_double(patch, pb->energy_per_cycle_series_patch);
  }
  buffer_push_double(patch, pb->power_tick_series_patch);
  buffer_push_double(patch, pb->total_energy_patch);
  PACK_MEM(pb, level_patch, PATCH_LEVL);
  buffer_push_int(patch, pb->flags);
  Buffer out = pb->out_patch;
  patch_builder_reset(pb);
  return out;
}

static int compare_nandstate(const void* a, const void* b) {
  int nandA = ((NandState*)a)->id_nand;
  int nandB = ((NandState*)b)->id_nand;
  return nandA - nandB;
}

/*
 * Removes duplicated nandstate entries potentially created by the event queue
 * update.
 *
 * Since today it simply appends the "awaken" NAND state in the list, three
 * things can happen:
 * 1. The NAND was previously sleeping, in which case there's no problem.
 * 2. The NAND was already active, in which case we would have 2 nand entries in
 * the patch.
 * 3. The nand has been awaken twice (two socket events), in which case there
 * will also be more than one nand entry.
 *
 * For that, as of today we perform a post-processing step where we look for
 * repeatead nands and remove old versions. Since the number of NAND active
 * state is expected to be very small per tick, and the array is contiguous (ie
 * cache friendly), we perform a simple sorting followed by duplcation removal.
 */
void patch_builder_remove_duplicated_nand(PatchBuilder* builder) {
  int nActNew = arrlen(builder->arr_nand_state);
  if (nActNew > 0) {
    qsort(builder->arr_nand_state, nActNew, sizeof(NandState),
          compare_nandstate);
    int nAct = nActNew;
    nActNew = 0;
    NandState* pNxt = builder->arr_nand_state;
    for (int iAct = 1; iAct < nAct; iAct++) {
      int nand1 = pNxt[nActNew].id_nand;
      int nand2 = pNxt[iAct].id_nand;
      if (nand1 == nand2) {
        int cnt0 = pNxt[nActNew].activation_counter;
        int cnt1 = pNxt[iAct].activation_counter;
        pNxt[nActNew].next_value = -2;
      } else {
        pNxt[++nActNew] = pNxt[iAct];
      }
    }
    nActNew++;
    arrsetlen(builder->arr_nand_state, nActNew);
  }
}

/*
 * Schedule modification for each socket in the fanout of the wire.
 */
static void fanout_schedule(
    int* arr_offset,         /* Offset for wire_to_skt */
    SocketDesc* wire_to_skt, /* Sockets for each wire */
    int wire,                /* Wire that is changing */
    int new_value,           /* New value of that wire */
    ScheduleItem** items     /* Schedule array that is updated*/
) {
  assert(wire >= 0);
  int off = arr_offset[wire];
  int ns = arr_offset[wire + 1] - off;
  ScheduleItem item = {0};
  for (int i = 0; i < ns; i++) {
    item.dt_ticks = wire_to_skt[off + i].dt_ticks + 1;
    item.ev.socket = wire_to_skt[off + i].socket;
    item.ev.value = new_value;
    arrput(*items, item);
  }
}

static inline int upper_idx(int x) {
  if ((x & (x - 1)) == 0) return 31 - __builtin_clz(x);  // exact power
  return 32 - __builtin_clz(x);
}

static void add_energy(PatchBuilder* builder, double e, int T) {
  assert(T >= 1);
  assert(e >= 0);
  T += 2;
  int i1 = upper_idx(T);
  int i0 = i1 - 1;
  int g1 = 1 << i1;
  int g0 = 1 << i0;
  double s = g1 - g0;
  double a = (g1 - T) / s;
  /* E = C_{total} * V^2*/
  assert(i0 >= 1 && i1 < NRJ_BINS);
  //  printf("Added Enregy: %f over %d\n", e, T);
  // builder->nrj_patch[0] += 10.f;
  builder->nrj_patch[i1 - 1] += (1.0 - a) * e;
  builder->nrj_patch[i0 - 1] += a * e;
}

void patch_builder_dispatch(PatchBuilder* builder, SimState* state, int wire,
                            int new_value) {
  assert(wire >= 0 && wire < state->pulse_size);
  WirePulse* pulse = &state->pulses[wire];
  bool changes = pulse_unpack_vafter(*pulse) != new_value;
  if (!changes) {
    return;
  }
  int b1 = pulse_unpack_vafter(*pulse);
  int t1 = state->cur_tick + 1;
  int mod = state->tick_mod;
  int nxt_tick = (t1 % mod);
  int new_pulse = pulse_pack((UnpackedPulse){
      .before_value = b1,
      .after_value = new_value,
      .pulse_tick = nxt_tick,
  });
  PulseDiff pulseDiff = {
      .wire = wire,
      .xor_value = new_pulse ^ (*pulse),
  };
  // printf("pulse: w=%d t=%d v=%d\n", wire, nxt_tick, new_value);
  arrput(builder->arr_pulse_diff, pulseDiff);
  /* Checks for update on max pulse time. */
  int cur_max_time = state->max_pulse_time + builder->max_pulse_time_diff;
  int t_D = builder->dg->wprop[wire].max_delay;
  double pulse_energy = builder->dg->wprop[wire].pulse_energy;
  int my_time = state->cur_tick + t_D;
  if (my_time > cur_max_time) {
    builder->max_pulse_time_diff = my_time - state->max_pulse_time;
  }
  fanout_schedule(builder->wg->wire_to_skt_off, builder->wg->wire_to_skt, wire,
                  new_value, &builder->arr_schedule_item);
  add_energy(builder, pulse_energy, t_D);
}

// Smallest power of 2 >= x
static int upper_pow2(int x) {
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

#ifdef _MSC_VER
#include <intrin.h>
static inline int __builtin_clz(unsigned int x) {
  unsigned long index;
  _BitScanReverse(&index, x);
  return 31 - index;
}
#endif

void sim_add_ui_event(Sim* s, float act) {
  SimUiEvent ev = {0};
  ev.sound = act / 6.0;
  arrput(s->ui_events, ev);
}

/*
 * For each active NAND:
 *
 * IF NAND state is RUNNING:
 *   Reduce the activation time counter.
 *   IF counter reaches 0:
 *      - The pulseN for the wire is updated.
 *      - For each socket in the fanout of the gate:
 *          - Schedule update in EventQeue.
 *      - NAND is removed from the active list.
 * IF NAND state is PENDING:
 *   - Computes the expected new value from sockets.
 *   - IF value is the same as current value, do nothing.
 *   - IF value is different, set STATE to RUNNING and initialize activation
 *      counter.
 *
 *  A NAND activation is triggered when one of its sockets change state.
 *  A socket can only change state via the EventQueue.
 *
 *  Today the design is such that the counter is manually updated every frame,
 *  because I wanted to eventually add a mechanic that allows nand activation
 *  to be impacted by external factors such as (i) chip temperature and (ii)
 *  Energy consumption, but such mechanic is not implemented yet.
 *
 *  NAND states are :
 *
 *  IDLE
 *  Doing nothing.
 *
 *  PENDING
 *  Socket has just changed, and is evaluating if needs to be updated.
 *
 *  RUNNING
 *  Will change value after a few ticks, according to internal timer.
 *
 *  Mind that the actual array is not updated, but instead the array in the
 *  builder is updated by hand (ie there's a copy of everything).
 *
 */
void patch_builder_update_nandstate(Sim* sim, PatchBuilder* builder,
                                    SimState* state) {
  int nAct = state->active_count;
  int total_sockets = arrlen(builder->pg->skt);
  int num_nands = arrlen(builder->pg->nands);
  const NandState* prev_nands = state->nand_states;
  for (int iAct = 0; iAct < nAct; iAct++) {
    int i_nand = prev_nands[iAct].id_nand;
    int next_value = prev_nands[iAct].next_value;
    int de = prev_nands[iAct].de;
    int max_delay = prev_nands[iAct].max_delay;
    int prev_counter = prev_nands[iAct].activation_counter;

    // Bounds check for NAND index
    assert(i_nand >= 0 && i_nand < num_nands);

    if (next_value >= 0) {
      int nxtCounter = prev_counter - 10;
      if (nxtCounter <= 0) {
        int wire = builder->wg->drv_to_wire[i_nand];
        assert(wire >= 0);
        patch_builder_dispatch(
            builder, state, wire,
            next_value);  // dispatch can be done asynchronously
      } else {
        NandState nxt = (NandState){
            .id_nand = i_nand,
            .next_value = next_value,
            .max_delay = max_delay,
            .de = de,
            .activation_counter = nxtCounter,
        };
        arrput(builder->arr_nand_state, nxt);
      }
    } else {
      // Here we evaluate next value and wether it actually needs update!
      // Bounds check for socket indices
      assert(2 * i_nand + 1 < total_sockets);
      int sktA = state->skt_values[2 * i_nand + 0];
      int sktB = state->skt_values[2 * i_nand + 1];
      int value = builder->nand_lut[(sktA << 2) + sktB];
      int wire = builder->wg->drv_to_wire[i_nand];
      assert(wire >= 0);
      WirePulse* pulse = &state->pulses[wire];
      bool changes = pulse_unpack_vafter(state->pulses[wire]) != value;
      if (changes) {
        int gate_delay = builder->dg->gate_delay[wire];
        sim_add_ui_event(sim, gate_delay);
        NandState nxt = (NandState){
            .id_nand = i_nand,
            .next_value = value,
            .max_delay = 10 * gate_delay,
            .de = 1,
            .activation_counter = 10 * gate_delay,
        };
        arrput(builder->arr_nand_state, nxt);
        add_energy(builder, builder->k_gate_energy, gate_delay);
      }
    }
  }
}

/*
 * Advances the event queue in time.
 *
 * The events to be processed in time T are of only one type: socket update.
 * Basically a socket is update to a new value (via the socket state array) and
 * the NAND gate associated with this socket is activated (passes from IDLE ->
 * PENDING)
 *
 * Each socket event in the queue defined by a socket ID and a new value.
 *
 * The queue is not updated at all, instead a list of "update intent" is
 * created into a patch builder.
 *
 * For now, sockets to external system is not supported. The socket is read by
 * hand at the end of each cycle. Eventually we could add a system to make
 * in-circuit components that interact on-the-fly with the chip, but it would
 * be a bit too slow with the regular LUA api. Something like that would
 * require a WASM integration, which I might include later.
 *
 */
void patch_builder_handle_socket_events(PatchBuilder* builder,
                                        SimState* state) {
  int nand_sockets = 2 * arrlen(builder->pg->nands);
  int total_sockets = arrlen(builder->pg->skt);
  SocketEvent* skt_ev = NULL;
  int ne = -1;
  event_queue_get_current_events(&state->ev_queue, &ne, &skt_ev);
  //  Handles socket events.
  //  Basically a socket event is when a NAND gate input changes.
  for (int iev = 0; iev < ne; iev++) {
    SocketEvent ev = skt_ev[iev];
    int socket = ev.socket;
    int value = ev.value;
    assert(socket >= 0 && socket < total_sockets);
    int prevValue = state->skt_values[socket];
    arrput(builder->arr_queue_popped, ev);
    if (prevValue != value) {
      // TODO: need to add it in the diff instead!
      int xor_value = value ^ prevValue;
      SocketValueDiff skt_val_diff = {.skt = socket, .xor_value = xor_value};
      arrput(builder->arr_skt_diff, skt_val_diff);
      if (socket < nand_sockets) {
        /* The new state is computed in the next iteration because the inputs
           to the NAND are not updated until then. Here we just know that the
           inputs have changed. Here, the NAND enters the "AWAKE" state. */
        int id_nand = socket / 2;
        NandState newState = {
            .id_nand = id_nand,
            .next_value = -2,
            .activation_counter = -1,
            .max_delay = 0,
            .de = 0.0f,
        };
        arrput(builder->arr_nand_state, newState);
      } else {
        // This is an external socket: TODO notify external component
      }
    }
  }
}

int sim_get_pixel_error_status(Sim* sim, int pix) {
  int w = sim->w;
  int h = sim->h;
  int s = w * h;
  int l = pix / s;
  int idx = pix - l * s;
  assert(l < sim->nl);
  int* wire_img = sim->wg.wmap[l];
  int c = wire_img[idx];
  int v = pulse_unpack_vafter(sim->state.pulses[c]);
  if (v == 2) return STATUS_CONFLICT;
  if (v == 3) return STATUS_TOOSLOW;
  return 0;
}

void sim_reset_dirty_mask(Sim* sim) {
  uint32_t* mask = sim->pulse_dirty_mask;
  for (int i = 0; i < sim->dirty_mask_size; i++) {
    mask[i] = 0;
  }
}

Tex* sim_render_v2(Sim* sim, int tw, int th, Cam2D cam, float frame_steps,
                   float slack_steps, int hide_mask, bool use_neon) {
  renderv2_update_hidden_mask(sim->rv2, hide_mask);
  UpdateTexture(sim->pulse_tex, sim->state.pulses);
  renderv2_update_pulse(sim->rv2, sim->pulse_tex, sim->pulse_dirty_mask);
  double rtime = GetTime();
  float utime = sin(2 * rtime * M_PI);
  float gtime = frame_steps * 5;
  Times times = {
      .tick = sim->state.cur_tick,
      .slack = slack_steps,
      .utime = utime,
      .glow_dt = gtime,
  };
  int ns = sim->state.active_count;
  Tex* out = renderv2_render(sim->rv2, cam, tw, th, ns, frame_steps, sim->nidx,
                             sim->state.nand_states, use_neon, times);

  /* TODO: the reset should be done inside the rendering directly to avoid extra
   * cache*/
  sim_reset_dirty_mask(sim);
  return out;
}

void sim_dry_run(GameRegistry* r) {
  Image img = GenImageColor(8, 8, BLANK);
  Color* clr = img.data;

#define SETPIXEL(x, y) clr[y * 8 + x] = WHITE
  SETPIXEL(1, 1);
  SETPIXEL(1, 2);
  SETPIXEL(1, 3);
  SETPIXEL(3, 1);
  SETPIXEL(3, 3);
  SETPIXEL(4, 2);
  SETPIXEL(6, 2);
  RenderTexture2D rt = gen_render_texture(8, 8, BLANK);
  Texture tex = LoadTextureFromImage(img);
  BeginTextureMode(rt);
  draw_tex(tex);
  EndTextureMode();
  UnloadTexture(tex);
  LevelAPI api = {0};
  Status stat = status_ok();
  assert(stat.ok);
  Sim s;
  stat = sim_init(&s, 1, &img, &api, &rt);
  assert(stat.ok);
  assert(!sim_has_errors(&s));
  HSim hsim = wrap_sim(&s);
  for (int i = 0; i < 3; i++) {
    stat = hsim_nxt(&hsim);
    assert(stat.ok);
    Cam2D cam = {0};
    cam.sp = 1;
    float slack = 2;
    int hide_mask = 0;
    bool use_neon = true;
    float frame_steps = 20;
    Tex* rendered =
        sim_render_v2(&s, 50, 50, cam, frame_steps, slack, hide_mask, use_neon);
  }
  hsim_destroy(&hsim);
  sim_destroy(&s);
  UnloadRenderTexture(rt);
  UnloadImage(img);
#undef SETPIXEL
}

void level_api_add_pg(LevelAPI* api, PinGroup pg) { arrput(api->pg, pg); }
void level_api_add_port(LevelAPI* api, int width, const char* id, int type) {
  PinGroup pg = {0};
  pg.id = clone_string(id), pg.type = type;
  int y = 0;
  int n = arrlen(api->pg);
  if (n > 0) {
    int wg = arrlen(api->pg[n - 1].pins);
    // position of last pin of previous port
    y = api->pg[n - 1].pins[wg - 1].y;
    y += 4;
  }
  y += 4;
  for (int i = 0; i < width; i++) {
    int px = 0;
    int py = y + 2 * i;
    pg_add_pin(&pg, px, py);
  }
  arrput(api->pg, pg);
}

void level_api_destroy(LevelAPI* api) {
  int np = arrlen(api->pg);
  for (int i = 0; i < np; i++) {
    pg_destroy(&api->pg[i]);
  }
  arrfree(api->pg);
  if (api->u) {
    api->destroy(api->u);
  }
  *api = (LevelAPI){0};
}
