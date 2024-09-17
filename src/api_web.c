// Mock for the WEB version.
#include "api.h"
#include "font.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"
#include "utils.h"

static LevelDesc level_desc = {0};

static struct {
  double elapsed;
  int value;
  double clock_time;
  int cycle;
  bool stop;
} C = {0};

void ApiLoad() {
  level_desc.ilevel = 0;
  level_desc.num_components = 1;

  PinDesc* pd = calloc(1, sizeof(PinDesc));
  pd->num_conn = 2;
  pd->conn[0].type = CONN_OUTPUT;
  pd->conn[0].len = 1;
  pd->conn[0].name = "power_on_reset";
  pd->conn[1].type = CONN_OUTPUT;
  pd->conn[1].len = 1;
  pd->conn[1].name = "clock";
  level_desc.pindesc = pd;

  ExtComp* ec = calloc(1, sizeof(ExtComp));
  ec->ni = 0;
  ec->no = 2;
  ec->dirty = true;
  ec->wires_out_x[0] = 0;
  ec->wires_out_y[0] = 4;
  ec->wires_out_x[1] = 0;
  // offset of 4
  // spacing of 6 between wires
  // spacing of 2 between pins
  ec->wires_out_y[1] = 4 + 6 + 2;
  level_desc.extcomps = ec;
}

void ApiUnload() {
  // Nothing to be done here.
}

void ApiLoadLevel(int i) {
  // Nothing to be done here. There's a single level.
}

void ApiStartLevelSimulation() {
  C.elapsed = 0;
  C.value = 0;
  C.cycle = 0;
  C.stop = false;
  level_desc.extcomps[0].dirty = true;
}

void ApiStopLevelSimulation() {
  C.elapsed = 0;
  C.value = 0;
  C.cycle = 0;
  C.stop = true;
}

void ApiOnLevelDraw(RenderTexture2D target, float camera_x, float camera_y,
                    float camera_spacing) {
  // Nothing to be done here.
}

LevelDesc* ApiGetLevelDesc() { return &level_desc; }

void ApiUpdateLevelComponent(void* ctx, int ic, int* prev_in, int* next_in,
                             int* output) {
  int por = C.cycle < 2;
  output[0] = por;
  output[1] = C.value;
}

LevelOptions* ApiGetLevelOptions() { return NULL; }

TickResult ApiOnLevelTick(float dt) {
  // S.elapsed = S.elapsed + dt
  // while (S.elapsed > S.clock_time) and not self.stop do
  //   self.value = 1 - self.value
  //   S.elapsed = S.elapsed - S.clock_time
  //   if self.value == 1 then
  //     self.cycle = self.cycle + 1
  //   end
  //   self.dirty = true
  // end
  // Translating from lua:
  C.elapsed = C.elapsed + dt;
  while ((C.elapsed > C.clock_time) && (!C.stop)) {
    C.value = 1 - C.value;
    C.elapsed = C.elapsed - C.clock_time;
    if (C.value == 1) {
      C.cycle++;
    }
    level_desc.extcomps[0].dirty = true;
  }
  int por = C.cycle < 2;
  int clock_value = -1;
  if (level_desc.extcomps[0].dirty && (por == 0)) {
    clock_value = C.value;
  }
  bool clock_updated = clock_value >= 0;
  return (TickResult){
      .reset = por,
      .clock_value = clock_value,
      .clock_updated = clock_updated,
      .clock_count = C.cycle,
  };
}

void ApiLevelClock(int ilevel, int* inputs, bool reset) {
  // Nothing to do here.
}

void ApiSetClockTime(float clock_time) { C.clock_time = clock_time; }
