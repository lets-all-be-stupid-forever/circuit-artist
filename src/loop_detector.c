#include "loop_detector.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static int MAX_CIRCUIT_UPDATE_COUNT = 100;

void LoadLoopDetector(LoopDetector* ld, int nc) {
  ld->nc = nc;
  ld->num_updates = 0;
  ld->update_count = malloc(nc * sizeof(int));
  ld->wire_has_changed = malloc(nc * sizeof(bool));
  ld->wire_changed_stack = malloc(nc * sizeof(int));
  for (int i = 0; i < nc; i++) {
    ld->wire_has_changed[i] = false;
    ld->update_count[i] = 0;
  }
}

bool LoopDetectorNotify(LoopDetector* ld, int k) {
  // Actual wire Update has happened
  // Attention: wire update is differetn from nand update
  ld->update_count[k]++;
  // the purpose of wire_has_changed right now is to set update to 0
  if (!ld->wire_has_changed[k]) {
    ld->wire_has_changed[k] = true;
    ld->wire_changed_stack[ld->num_updates++] = k;
  }
  return ld->update_count[k] > MAX_CIRCUIT_UPDATE_COUNT;
}

void UnloadLoopDetector(LoopDetector* ld) {
  free(ld->update_count);
  free(ld->wire_has_changed);
  free(ld->wire_changed_stack);
}

void LoopDetectorReset(LoopDetector* ld) {
  for (int i = 0; i < ld->num_updates; i++) {
    int w = ld->wire_changed_stack[i];
    ld->wire_has_changed[w] = false;
    ld->update_count[w] = 0;
  }
  ld->num_updates = 0;
}

bool LoopDetectorIsWireLooping(LoopDetector* ld, int k) {
  return ld->update_count[k] > MAX_CIRCUIT_UPDATE_COUNT / 2;
}
