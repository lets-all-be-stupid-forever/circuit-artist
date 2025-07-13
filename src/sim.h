#ifndef SIM_H
#define SIM_H
#include "img.h"
#include "loop_detector.h"

// Maximum number of wires per simulation external component (or chip).
#define MAX_SLOTS 128

// The 4 possible values for a wire by default
typedef enum {
  BIT_0,
  BIT_1,
  BIT_UNDEFINED,
  BIT_BUGGED,
} WireStateType;

// Describes the interface of an External Component.
//
// External components are entities that are updated outside of the NAND-image
// logic but interact with the system as if they were external chips. The way
// it works is, the wire values in the image are simulated, then the external
// components are updated, then their output wire values are injected again in
// the image which is re-simulated, and back-and-forth until there's no change.
//
// In practice, these external components are written in lua. For now, the
// interface is always in the left-side of the image, but in theory it could be
// anywhere within the image.
//
// Each wire (input or output) is has a pixel mapped to it, from which it will
// read/write the wire state.
typedef struct {
  // Number of input wires.
  int ni;
  // Number of output wires.
  int no;
  // Dirty flag for the component. A dirty component is always updated during
  // simulation. Otherwise, it is only updated when there's a change in in
  // input.
  bool dirty;
  // X pixel coordinate of each input wire.
  int wires_in_x[MAX_SLOTS];
  // Y pixel coordinate of each input wire.
  int wires_in_y[MAX_SLOTS];
  // X pixel coordinate of each output wire.
  int wires_out_x[MAX_SLOTS];
  // Y pixel coordinate of each output wire.
  int wires_out_y[MAX_SLOTS];
} ExtComp;

// Parsed image before simulation.
// Represents the NAND/wire topology.
typedef struct {
  // Width of the parsed image.
  int width;
  // Height of the parsed image.
  int height;
  // Image of the same size as the original. Describes what each pixel in the
  // image represent: 0 is background. 1 is a wire, 2 is a NAND
  int* image;
  // Total number of connected components.
  int nc;
  // ID of each connected component.
  int* comp;
  // total number of detected nands.
  int num_nands;
  // 3 values for each nand, mapping pixels, not components:
  //   3n+0 -> Pixel of the first input of the nand
  //   3n+1 -> Pixel of the second input of the nand
  //   3n+2 -> Pixel of the output of the nand
  int* nands;
  // Time spent parsing the image in seconds.
  double time_parsing;
  // Offset of pixels per component (used to extract size as well).
  int* comp_off;
  // Pixel index of each component.
  int* comp_pixels;
  // Total number of nands found.
  int num_nand_pixels;
  // x,y pixels that belong to nands.
  // Used for coloring the nand pixels with a specific color.
  int* nand_pixels;
  // A reference to the original image, where we can access its pixel values.
  Image original_image;
} ParsedImage;

// Callback called when an external component should be updated.
//
// Interface:
//   ctx: a pointer to a context passed in the construction of the simulator.
//   c: index of the component (0 = first component, 1=second component etc).
//   prev_in: (input) bit values of the input wires before the update (before
//      input change).
//   next_in: (input) bit values of the input wires during the update (after
//      input change).
//   next_out: (output) bit values of output wires after the update. Needs to
//      be filled during the update. It's not pre-filled.
//
// For example, when the input of a component change, the update function will
// be called with `prev_in` containing the bit values before modification, and
// `next_in` the new modified values. Then, the update function is responsible
// for filling the `next_out` array, which is then translated back to the
// simulated image's respective wires.
//
// The number of wires and pixel coordinate of each wire is represented in the
// ExtComp struct. Mind that the interface here is at the bit/pin level,
// whereas from lua we might interact at the "group of wires" level (for
// example, 8-bit connections).
//
// Mind that this callback is used for all components, there's no per-component
// callback. It's up to the callback to route the logic to each component.
typedef void (*ComponentUpdateCallback)(void* ctx, int c, int* prev_in,
                                        int* next_in, int* next_out);

// Simulation context.
//
// The way it works is:
// Each wire has a state.
// If the wire input is 0, it means it is not linked to a nand.
// The 0 wire never changes.
// If it has valid wires as input, it has
// The updates are push-based: once a wire has changed, it triggers the update
// for all other wires for which it is an input.
//
// We keep a lookup table for that, stored in nfo/fo list (fan-out)
//
// 1. When parsing, first we detect the nands.
//
// 2. Everything that is not a NAND is considered a wire.
//   Wires are formed by "connected" pixels. So each pixel is either a NAND a
//   wire.
//
// 3. After that, we use the NAND placements to determines the 2 inputs of each
// wire.
//    Note that we don't accept more than one NAND outputting to the same wire,
//    because it breaks this assumption.
//
// 4. We initiate every wire in UNDEFINED state.
//
// 5. Then we run a first simulation where we trigger every wire that doesnt
//    have input (ie, end wires). They are triggered to 0.
//
//  The triggering works via a "update" queue, or event queue.
//
//  We don't keep track of actual components, just wires, since we only have
//  NANDs in the engine.
//
typedef struct {
  // Parsed image used for creation (reference only).
  // Since it keeps a reference to the original image, it can be useful for
  // accessing original pixel values during the rendering of the simulated
  // images.
  ParsedImage pi;
  // Total number of wires.
  int nc;
  // Total number of nands.
  int nn;
  // NAND graph of the circuit.
  // Each nand has 2 inputs and 1 output.
  // Has size `3*n_nand`, 2 inputs and 1 output for each NAND.
  // Each wire can have more than 1 NAND connected to it, even though it might
  // lead to buggy solutions.
  // The update formula of the system is:
  //
  // For each updated NAND:
  //    WireState[graph[3*i+2]] := NAND(WireState[graph[3*i+0]],
  //    WireState[graph[3*i+1]])
  //
  int* graph;
  // Wire state of each wire.
  // The wire 0 is special, it doesnt actually map to a pixel on the image and
  // represents a constant value of 0. it is also never updated.
  // Has size `nc`.
  int* state;
  // Cached state of each component the last time the circuit was rendered.
  // It is used to accelerate the circuit rendering: we only update the pixels
  // for the wires that had a change in the wire state.
  // Has size `nc`.
  int* last_render_state;
  // Number of events in the ingress event queue.
  // Event queues are how we handle changes in state. An item in the queue
  // consists of a pair (wire_number, new_wire_value).
  int ne;
  int ne_swap;
  // Max allowed number of ingress events in the event queue. If we receive a
  // number of event higher than this value, they are ignored. It's there
  // mostly to avoid potential memory corruption/overflow, in practice it's
  // very hard to reach this limit.
  int max_events;
  // Ingress event queue. (ie, events that come from the user or manually
  // triggered through the UI). These don't come from external components.
  // It's used for example when the user clicks on a wire to toggle it. In that
  // case an event is created, which is resolved in the next call to simulate.
  // Has size `2*max_events`.
  int* ev;
  int* ev_swap;
  // Fanout offset list.
  // ie, the fanout of the wire `i` is represented by the wires
  // fo[nfo[i]], fo[nfo[i]+1], ..., fo[nfo[i+1]], where `fo` is the fanout
  // list.
  // Has size `nc`.
  int* nfo;
  // Fanout list for each wire.
  // The fanout of a wire is the list of wires that has this wire as input. So
  // when you're updating the wires in the circuit, after updating a given
  // wire, you want to update all the wires on its fanout.
  // It stores the list of fanouts one from each wire one after the other. You
  // need the  fanout list offset (nfo) array to access the fanouts for a given
  // wire.
  // Has maximum size of `2*nc`.
  int* fo;
  // Place in the queue where the given circuit is queued.
  // Buffer used during the call to simulate. Allows us to avoid queueing event
  // of the same wire more than once per simulation step.
  // Has size `nc`.
  int* queued_at;
  // Rendered simulated image at different resolutions (pyramid).
  // These images are updated at each call to SimGenImage().
  Image simulated[3];
  // Loookup table for the NAND gate. nand(a, b) = nand_lut[a][b]
  // We keep a lut so we can quickly do things like NAND with undefined bits.
  // It's 4x4 because we consider 4 states for a wire.
  int nand_lut[4 * 4];
  // Number of external components attached to the simulation context.
  // It requires at least one component to run.
  int necomps;
  // List of external components.
  // Has size `necomps`.
  ExtComp* ecomps;
  // Array storing the input wire state of each external component.
  // Has an offset of MAX_SLOTS for each component.
  // Has size MAX_SLOTS * necomps.
  int* ecomp_inputs;
  // Array storing the output wire state of each external component.
  // Has an offset of MAX_SLOTS for each component.
  // Has size MAX_SLOTS * necomps.
  int* ecomp_outputs;
  // Context to be sent to update_cb callback.
  // Note that it's a single context for all external components.
  void* update_ctx;
  // Callback function called when an external component needs to be updated.
  ComponentUpdateCallback update_cb;
  // Time spent during parsing of the original image in seconds.
  double time_parsing;
  // Total time spent creating the circuit (after the parsing).
  double time_creation;
  // Time spent rendering the simulated image in the last call to
  // SimGenImage().
  double time_gen_image;
  // Number of updates in the last simulate.
  int num_updates_last_simulate;
  // Total cumulated number of updates since start.
  int total_updates;
  int state_w;
  int state_h;
  float* state_buffer;
  Texture2D t_comp_x;
  Texture2D t_comp_y;
  int istate;
  Texture2D t_state[2];
  LoopDetector loop_detector;

  // Whether to use delay in next_update_delay.
  // A time of 0 means the simulation is "synchronous", ie, it will update
  // until there's no change left.
  bool use_delay_time;

  // Delay time to be used
  float nand_activation_delay;

  // Time to wait until we can run a next simulation update.
  // Used for detailed simulation.
  float next_update_delay;

  bool needs_update_state_texture;

  // If true, it means a loop has been detected during simulation.
  bool is_looping;
  // Flag to see if the wire has changed during an event update
  bool* ev_changed;
} Sim;

// Parses a raw image into an intermediate data structure to be fed into the
// simulation. This step can be time consuming in case of big images.
ParsedImage ParseImage(Image image);

// Destructor
void UnloadParsedImage(ParsedImage p);

// Creates a simulation context.
// It takes as input an instance of a parsed image and specification of the
// external components that are associated to the simulated image.
void SimLoad(Sim* s, ParsedImage pi, int necomps, ExtComp* ecomps,
             ComponentUpdateCallback update, void* comp_ctx);

// Destructor
void SimUnload(Sim* s);

// Runs a simulation step until convergence.
//
// This is the main simulation routine  It will alternate between simulating
// the image and the external components until convergence. It takes as input
// two things: (i) the events coming from user interaction or manually queue
// somehow and (ii) the dirty flag of external components.
//
// For example, when the user clicks on a wire during simulation, an event will
// be queued in the event queue, which will be used to start simulation update
// cascade at the call to simulate. On the other hand, an external component
// associated with a clock mechanism will set its state to "dirty" at the clock
// frequency rate to trigger the update here.
//
// If there's no dirty components or event queued, nothing is done.
//
// The Simulation Algorithm
// ------------------------
// The simulation is a simplified version of an event-driven tri-state logic
// circuit simulation where you only have NAND gates. Tri-state as in each wire
// can be in 3 different states (i) binary 0, (ii) binary 1, (iii) undefined.
// This is the lower level simulation that is applied to the wires/nands in the
// image.
//
// Then, there's another layer on top of it that alternates between the image
// and the external components until convergence, as in the pseudo-code below:
//
// FUNCTION simulate(wire_state, wire_graph, events, comps)
//    REPEAT:
//       WHILE events IS NOT EMPTY DO
//          wire_state, events := SIMULATE_EVENT_STEP(wire_state, wire_graph,
//          events)
//       ENDWHILE
//       FOR EACH COMPONENT comp IN comps DO
//          IF (comp IS DIRTY) OR (COMP_INPUT_HAS_CHANGED(comp, wire_state))
//          THEN
//              SIMULATE_COMP(comp, wire_state)
//              IF COMP_OUTPUT_HAS_CHANGED(comp, wire_state) THEN
//                 events = events + DIFF(comp_next_output, comp_prev_output)
//              ENDIF
//          ENDIF
//        ENDFOR
//        IF events is EMPTY THEN
//          STOP
//        ENDIF
//    ENDREPEAT
// ENDFUNC
//
void SimUpdate(Sim* s, float time_budget, float* time_used,
               bool* use_delay_time);

// Enqueues an event that will toggle (invert) the wire value at the exact
// pixel position (x, y).
// It's used when the user clicks at the image during simulation.
void SimTogglePixel(Sim* s, int x, int y);

// Finds the closest wire to a pixel position.
// It's useful to find which wire to toggle whenever the user clicks somewhere
// in the image. Indeed, if we don't do that it becomes very hard to click on
// any wire because there's many more background pixels than wire pixels in the
// image, specially when the circuit is scaled up.
void SimFindNearestPixelToToggle(Sim* s, int tol, float x, float y, int* px,
                                 int* py);

// Renders the simulated image.
//
// It basically reads the wire state of each wire pixel and sets the pixel
// value of that pixel in function of the bit state. Normally we put brighter
// pixels for 1's and darker pixels for 0's. The rendered image has same size
// as the original input image. Mind that there's no actual memory allocation
// here, only a change in pixel values.
//
// Normally called once per draw frame to display the circuit state to the user.
// Note that at each game frame we can run multiple simulate calls but we don't
// need to render the image at each time, we only need to do it at the FPS rate.
// This method has a non-trivial computing time, specially when there are
// multiple wires changing at the same time.
void SimGenImage(Sim* s);

// Runs the update call of a single component and queues the events triggered
// by it, without running the complete simulation loop in the image.
// It's useful when we have components with memory: we first dispatch/run the
// clock component update so it can "trigger" the clock event in external
// components so they can update their internal state.
//
// Otherwise, if we rely on clock being manually passed to the component, it
// will have significant propagation delay and the memory will be out of sync
// with the memory designed in the image. This happens because the external
// components only update after everything has finished updating in the image,
// while an ideal implementation would make the call to update() in the
// component at the exact moment when any input has changed, which might
// trigger way too many calls to lua and might significantly slow down the
// simulation overall.
void SimDispatchComponent(Sim* s, int icomp);

// Returns a pointer to the input bits of a given component.
int* SimGetComponentInputs(Sim* s, int icomp);

// Returns a pointer to the output bits of a given component.
int* SimGetComponentOutputs(Sim* s, int icomp);

// Returns the number of NAND gates parsed in the simulation.
int SimGetNumNands(Sim* s);

// Returns the wire state (bit) at the pixel image coordinate (x,y)
int SimGetWireValue(Sim* s, int x, int y);

// Returns the default color for a pixel in the image when the wire has state
// "v" and pixel color "c". It's used for displaying wire interface in the left
// side of the image with the same color as in the original image.
Color GetSimuColorOnWire(Color c, int v);

// Get default color value for a wire when we don't know its pixel color.
Color GetWireColor(int wire_value);

bool SimIsBusy(Sim* s);

#endif
