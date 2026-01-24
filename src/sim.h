#ifndef CA_SIM_H
#define CA_SIM_H
#include "common.h"
#include "dist_graph.h"
#include "event_queue.h"
#include "game_registry.h"
#include "hsim.h"
#include "level_api.h"
#include "paged_stack.h"
#include "pixel_graph.h"
#include "renderv2.h"
#include "series.h"
#include "status.h"
#include "tex.h"
#include "wire_graph.h"

#define NRJ_BINS 32

/*
 * Contains value before, after and the time of activation.
 * (see UnpackedPulse)
 */
typedef int WirePulse;
struct Sim;

enum {
  S_BIT_ZERO,
  S_BIT_ONE,
  S_BIT_UNDEFINED,
  S_BIT_BUGGED,
};

typedef struct {
  int dt_ticks;
  SocketEvent ev;
} ScheduleItem;

typedef struct {
  int before_value;
  int after_value;
  int pulse_tick;
} UnpackedPulse;

typedef struct {
  i64 b;
  i64 f; /* flag for undefined*/
} PinComm;

typedef struct {
  /* Serialized states */
  int* skt_values;                /* Current value of sockets (size=numSkts); */
  EventQueue ev_queue;            /* Event Queue for Socket updates */
  WirePulse* pulses;              /* Visu state of wires (size=numWires) */
  NandState* nand_states;         /* State of NAND gates. (max_size=numNands) */
  Series power_tick_series;       /* List of spent energy per tick (power) */
  Series energy_per_cycle_series; /* List of spent energy per cycle */
  Series ticks_per_cycle_series;  /* List of ticks per cycle */
  float energy_t[NRJ_BINS];       /* Energy for each time horizon T=2^k */
  double power;                   /* Total power at this timestep */
  double acc_nrj;
  double total_energy; /* Total used energy */
  int max_pulse_time;  /* Time after which pulses are propagated. */
  int active_count;    /* size of active nand state array */
  int cur_tick;        /* current simu tick */
  int max_tick;        /* max num of ticks in a clock cycle */
  int cycle;  /* Proxy to clock: everytime level is updated this is increased */
  bool error; /* Simulation is in Error mode */
  bool done;  /* Simulation is in Done mode */

  float pow_decay[NRJ_BINS];
  int pulse_size; /* actual size of the pulse array */
  struct Sim* sim;

  int tick_mod;
  int tick_slots;
} SimState;

/* (B0, T0, A0) --> (A0, T1, A1)
 *  Forward:
 *   T1 = XOR(xor_t, T0)
 *   A1 = XOR(xor_value, B0)
 *
 *  Backward:
 *  B0 = XOR(xor_value, A1)
 *  T0 = XOR(xor_t, T1)
 */
typedef struct {
  int wire;
  int xor_value;
} PulseDiff;

/* (V0) --> (V1)
 * Forward:
 *   V1 = XOR(xor_value, V0)
 * Backward:
 *   V0 = XOR(xor_value, V1)
 */
typedef struct {
  int skt;
  int xor_value;
} SocketValueDiff;

typedef struct {
  NandState* arr_nand_state;
  SocketValueDiff* arr_skt_diff;
  PulseDiff* arr_pulse_diff;
  SocketEvent* arr_queue_popped;
  ScheduleItem* arr_schedule_item;
  int* arr_wire_to_shift;
  Buffer level_patch;
  double power_patch;
  double total_energy_patch;
  double power_tick_series_patch;
  double energy_per_cycle_series_patch;
  double ticks_per_cycle_series_patch;
  int max_tick_patch;
  float nrj_patch[NRJ_BINS]; /* XOR for each energy.  */
  double acc_nrj_patch;

  // only used in unpacked version
  int arr_nand_state_len;
  int arr_skt_diff_len;
  int arr_pulse_diff_len;
  int arr_queue_popped_len;
  int arr_schedule_item_len;
  int flags;

  // FanoutGraph* fg; /* Does not own */
  WireGraph* wg;  /* Does not own */
  DistGraph* dg;  /* Does not own */
  PixelGraph* pg; /* Does not own */

  int max_pulse_time_diff;
  int nand_lut[16];     /* Fixed array used in nand evaluation */
  bool cycle;           /* Wether it increases cycle */
  Buffer out_patch;     /* Buffer where patch is compiled to */
  bool level_updated;   /* Whether level needs update */
  double k_gate_energy; /* Energy released in gate activation */
  double k_vdd_square;
} PatchBuilder;

typedef struct {
  float sound;
} SimUiEvent;

/*
 * the first drivers and sockets belong to the NANDs.
 * Then, the next belong to external wires, and the last belong to lone wires.
 */
typedef struct Sim {
  /* Parsing */
  int nl;             /* Number of layers */
  int w;              /* Width in each layer */
  int h;              /* Height in each layer */
  int num_wire;       /* Num of wires (ie connected components) */
  DistSpec dist_spec; /* Parameters to compute wire speed and others */
  PixelGraph pg;      /* Simplest graph (pixel nodes) */
  WireGraph wg;       /* Graph Organized by components (wires) */
  DistGraph dg;       /* Wire distances */
  // FanoutGraph fg;     /* Fanout information */
  int* switch_delay;  /* load capacitance per nand */
  int* switch_energy; /* wire length rc delay */

  /* Simulation */
  bool poked;      /* Wheter user has toggled any wire */
  PinComm* pinbuf; /* Numbers used in pin communicaation with level */
  SimState state;  /* Simulation state */
  PatchBuilder patch_builder; /* Temporary buffer used during state update */

  Tex* light_ema;
  Tex* circ_ema;
  LevelAPI* api;

  /* Rendering */
  NandDesc* nidx;    /* index of each nand (used in visu) */
  Texture pulse_tex; /* Pulse GPU data (used in visu)*/

  Cam2D prv_cam;
  SimUiEvent* ui_events;
  RenderV2* rv2;
  uint32_t* pulse_dirty_mask;
  int dirty_mask_size;
  bool pause_requested; /* pausing from within simulation */
  int64_t update_interval;
  int base_tps;
} Sim;

Status sim_init(Sim* sim, int nl, Image* img, LevelAPI* api,
                RenderTexture2D* layers);
void sim_destroy(Sim* sim);
Tex* sim_render_v2(Sim* sim, int tw, int th, Cam2D cam, float frame_steps,
                   float slackSteps, int hide_mask, bool use_neon);
Tex* sim_render_energy(Sim* sim, int tw, int th);

bool sim_is_idle(Sim* sim);
void sim_find_nearest_pixel(Sim* sim, int tol, v2 fPix, int* pix);
bool sim_has_errors(Sim* sim);
void sim_toggle_pixel(Sim* sim, int pix); /* Used for manual interaction */
int sim_get_pixel_error_status(Sim* sim, int pix);
HSim wrap_sim(Sim* sim);
void sim_port_write(Sim* sim, int iport, PinComm pc);
PinComm sim_port_read(Sim* sim, int iport);

void sim_state_destroy(SimState* state);
void sim_state_patch_forward(SimState* state, Buffer patch);
void sim_state_patch_backward(SimState* state, Buffer patch);
bool sim_state_has_work(SimState* state);
void sim_state_step_patch(SimState* state, Buffer patch);
bool sim_state_has_work(SimState* state);
int sim_state_get_max_tick(SimState* state);
float sim_get_pixel_dist(Sim* sim, int pix);

/* These are the 3 most important functions of state management. */
void patch_builder_update_nandstate(Sim* sim, PatchBuilder* builder,
                                    SimState* state);
void patch_builder_handle_socket_events(PatchBuilder* builder, SimState* state);
void patch_builder_remove_duplicated_nand(PatchBuilder* builder);

// Interface for creating a patch
void patch_builder_reset(PatchBuilder* pb);
void patch_builder_destroy(PatchBuilder* patch_builder);
bool patch_builder_empty(PatchBuilder* pb);
Buffer patch_builder_commit(PatchBuilder* patch_builder, SimState* state);

void patch_builder_dispatch(PatchBuilder* builder, SimState* state, int wire,
                            int new_value);

static inline int pulse_pack(UnpackedPulse p) {
  return p.before_value | (p.after_value << 2) | (p.pulse_tick << 4);
}

/*
 * bv = before_value
 * av = after_value
 * ti = pulse_tick
 */
static inline int pulse_pack2(int bv, int av, int ti) {
  return bv | (av << 2) | (ti << 4);
}

static inline UnpackedPulse pulse_unpack(int p) {
  return (UnpackedPulse){
      .before_value = p & 0b11,
      .after_value = (p >> 2) & 0b11,
      .pulse_tick = (p >> 4),
  };
}

static inline int pulse_unpack_vafter(int p) { return (p >> 2) & 0b11; }
static inline int pulse_unpack_vbefore(int p) { return p & 0b11; }
static inline int pulse_unpack_tick(int p) { return p >> 4; }

void sim_dry_run(GameRegistry* r);

#endif
