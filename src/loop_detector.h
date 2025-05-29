#ifndef LOOP_DETECTOR_H
#define LOOP_DETECTOR_H
#include "stdbool.h"

// Detects looping wire within same clock cycle during synchronous simulation.
typedef struct {
  int num_updates;
  // Update count of each wire.
  // Allows the detection of loops: a wire that has been updated too many times
  // is identified as a buggy loop wire.
  // Has size `nc`.
  int* update_count;
  // Flag describing whether a given wire has changed state during a simulation
  // (simulate) step. Used for tracking which wires have updated in a simulation
  // step. Only used during the call to `simulate`. Has size `nc`.
  bool* wire_has_changed;
  // Total number of wires.
  int nc;
  // Stack containing the wires that have changed during simulation.
  // It is used to avoid iterating through all the wires to find out those who
  // have changed at each iteration.
  // Has max size `nc`.
  int* wire_changed_stack;
} LoopDetector;

void LoadLoopDetector(LoopDetector* ld, int num_wires);
// Notifies that a wire has changed.  Returns
// true if a loop was detected.
bool LoopDetectorNotify(LoopDetector* ld, int wire);
bool LoopDetectorIsWireLooping(LoopDetector* ld, int wire);
void LoopDetectorReset(LoopDetector* ld);
void UnloadLoopDetector(LoopDetector* ld);

#endif
