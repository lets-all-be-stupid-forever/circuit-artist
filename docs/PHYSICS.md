# Circuit Simulation Physics

This document describes the circuit simulation engine used in Circuit Artist. The simulation models realistic wire delays and power consumption using established techniques from electronic design automation (EDA) and VLSI design.

## Table of Contents

1. [Overview](#overview)
2. [Building the Graph from Pixels](#building-the-graph-from-pixels)
3. [The PI Model for Wire Segments](#the-pi-model-for-wire-segments)
4. [From Pixel Graph to RC Graph](#from-pixel-graph-to-rc-graph)
5. [Elmore Delay for RC Trees](#elmore-delay-for-rc-trees)
6. [Handling Cyclic Graphs](#handling-cyclic-graphs)
7. [Delay Interpolation Between Nodes](#delay-interpolation-between-nodes)
8. [Gate Input Capacitance (Fanout)](#gate-input-capacitance-fanout)
9. [Gate Activation and Wire Propagation](#gate-activation-and-wire-propagation)
10. [Fixed Gate Delay](#fixed-gate-delay)
11. [Maximum Wire Delay](#maximum-wire-delay)
12. [Energy Computation](#energy-computation)
13. [Event-Driven Simulation](#event-driven-simulation)
14. [Circular Event Queue](#circular-event-queue)
15. [NAND Gate States](#nand-gate-states)
16. [Wire Pulses and Visualization](#wire-pulses-and-visualization)
17. [Time-Travel with Delta Patches](#time-travel-with-delta-patches)
18. [Limitations](#limitations)
19. [References](#references)

---

## Overview

The simulation engine transforms pixel art circuits into realistic timing simulations. The key insight is that wire delays in integrated circuits scale quadratically with length (due to RC delay), not linearly. This creates interesting optimization challenges for players designing efficient circuits.

The simulation pipeline consists of:

1. **Parsing**: Convert pixel images to a graph representation
2. **Analysis**: Compute RC delays using the Elmore delay model
3. **Simulation**: Run event-driven logic simulation with accurate timing

---

## Building the Graph from Pixels

The first step converts layered 2D images into a graph structure suitable for circuit analysis.

### Pixel Classification

Each pixel is classified based on its 4-neighborhood connectivity pattern:

```
    U (up)
  L M R (left, middle, right)
    B (bottom)
```

Pixels are categorized as:

- **Wire pixels**: Foreground pixels (non-transparent)
- **Background pixels**: Transparent pixels
- **Via pixels**: Single pixels connecting to upper layers
- **NAND pixels**: Special 3x3 patterns detected as logic gates

The NAND pixels are extracted from the bottom layer before the wires are parsed.

### Sockets and Drivers

We call `socket` the pixel/slot where the nand input "plugs" in and `driver` the slot it plugs into. For example, each nand will have 2 `socket` pixels with given coordinates, and 1 `driver` pixel.

This concept is then extended in levels: each level will define it's socket pixels and driver pixels, which is further integrated in the simulation. Then, from the `lua` script code, these sockets work as if they were inputs to real nands, and drivers as if they were output of nands, but they actually interact with lua code.

The lua code can read wire values from sockets and write/trigger wire values from its drivers, defined at the script initializaton..

### Graph Node Types

To handle both horizontal and vertical wire segments efficiently, each pixel can have up to two graph nodes:

- **Horizontal segment node** (positive index): `idx = y * width + x + layer * width * height`
- **Vertical segment node** (negative index): `idx = -(horizontal_idx) - 1`

This dual-node representation correctly handles cross intersections where horizontal and vertical wires meet but don't electrically connect.

### Edge Types

The graph contains four types of edges:

1. **Horizontal edges**: Connect adjacent pixels in the same horizontal wire segment
2. **Vertical edges**: Connect adjacent pixels in the same vertical wire segment
3. **H-V junction edges**: Connect horizontal and vertical nodes at T-junctions and L-corners (zero length)
4. **Layer edges**: Connect via pixels to the upper layer (length = 1 pixel equivalent)

### Connected Components

After building the graph, a Union-Find algorithm identifies connected components. Each connected component represents a single electrical wire (net).

---

## The PI Model for Wire Segments

The simulation uses the **PI model** (also called the lumped capacitance model) for wire segments, a standard approximation in VLSI timing analysis.

In the PI model, each wire segment's distributed capacitance is split between its two endpoints:

```
    R_segment
o----/\/\/\----o
|              |
C/2           C/2
|              |
GND           GND
```

For a wire segment of length $w$:

$$R_{seg} = w \cdot R_{layer}$$

$$C_{seg} = w \cdot C_{layer}$$

Where $R_{layer}$ and $C_{layer}$ are the resistance and capacitance per unit length for each metal layer.

**Reference**: The PI model is described in detail in Rabaey et al., _Digital Integrated Circuits: A Design Perspective_ (2003), Chapter 4.

---

## From Pixel Graph to RC Graph

The pixel graph is transformed into an RC (resistance-capacitance) network for timing analysis.

### Layer-Dependent Parameters

Different metal layers have different electrical properties. Upper layers typically have lower resistance (thicker wires) but higher capacitance (larger area):

| Layer      | $RC_{per\_pixel}$ | $C_{per\_pixel}$ | Relative Speed |
| ---------- | ----------------- | ---------------- | -------------- |
| 0 (bottom) | 1.0x              | 1.0x             | Slowest        |
| 1 (middle) | 0.5x              | 1.5x             | Medium         |
| 2 (top)    | 0.25x             | 2.0x             | Fastest        |

The effective RC product determines the delay, so upper layers propagate signals faster despite higher capacitance.

### Effective RC at Segment Junctions

When two segments from different layers connect (via a via), the effective R and C are averaged:

$$R_{eff} = \frac{R_{layer_u} + R_{layer_v}}{2}$$

$$C_{eff} = \frac{C_{layer_u} + C_{layer_v}}{2}$$

---

## Elmore Delay for RC Trees

For tree-structured RC networks (a single driver with no loops), the **Elmore delay** provides an efficient closed-form approximation for signal propagation time.

**Reference**: W.C. Elmore, "The Transient Response of Damped Linear Networks with Particular Regard to Wideband Amplifiers," _Journal of Applied Physics_, vol. 19, pp. 55-63, January 1948.

### The Elmore Delay Formula

For a node $k$ in an RC tree, the Elmore delay from the root (driver) is:

$$T_D^{(k)} = \sum_{i \in path(root \to k)} R_i \cdot C_{downstream}^{(i)}$$

Where $C_{downstream}^{(i)}$ is the total capacitance of the subtree rooted at node $i$.

### Two-Pass Algorithm

The implementation uses a two-pass algorithm:

**Pass 1 - Downstream Capacitance (leaf to root):**

```
for each node u in reverse topological order:
    C_down[u] = C_self[u]  // node's own capacitance
    for each child v of u:
        C_down[u] += C_down[v]
```

**Pass 2 - Elmore Delay (root to leaf):**

```
delay[root] = 0
for each node u in topological order:
    for each child v of u:
        R_edge = R_per_length * edge_length(u,v)
        delay[v] = delay[u] + R_edge * C_down[v]
```

The total capacitance at the root (`C_down[root]`) is also used to compute the gate activation delay.

---

## Handling Cyclic Graphs

Real circuits may contain wire loops (cycles), but the Elmore delay formula only applies to trees. The simulation uses an approximation for cyclic graphs:

### Algorithm

1. **Extract a spanning tree**: Use Dijkstra's algorithm weighted by capacitance to find the minimum-capacitance spanning tree rooted at the driver.

2. **Compute Elmore on the tree**: Apply the standard Elmore algorithm to the spanning tree.

3. **Scale for total capacitance**: The pruned edges still contribute capacitance. Scale all delays by the ratio:

$$scaling = \frac{C_{total\_graph}}{C_{spanning\_tree}}$$

This ensures that the total energy (proportional to total capacitance) is conserved, even though the exact delay distribution is approximate.

### Dijkstra-Based Tree Extraction

```
djikstra_spanning_tree(graph, root):
    tree_edges = []
    visited = set()
    priority_queue.push(root, cost=0)

    while priority_queue not empty:
        u, prev = priority_queue.pop()
        if u in visited: continue
        visited.add(u)

        if prev != null:
            tree_edges.add(prev -> u)

        for each edge (u -> v) with capacitance c:
            if v not visited:
                priority_queue.push(v, prev=u, cost=c)

    return tree_edges
```

---

## Delay Interpolation Between Nodes

Pixel-level visualization requires interpolating delays between graph nodes. The simulation uses a quadratic interpolation formula that accounts for the distributed RC nature of wire segments.

For a pixel at position $p$ between nodes at positions $p_0$ and $p_1$ with delays $d_0$ and $d_1$:

$$a = \frac{p - p_0}{p_1 - p_0}$$

$$d(p) = d_0 + (d_1 - d_0) \cdot a + RC_{seg} \cdot a \cdot (1-a)$$

The quadratic term $RC_{seg} \cdot a(1-a)$ accounts for the internal RC delay within the segment itself, where:

$$RC_{seg} = \frac{1}{2} \cdot w^2 \cdot R_{per\_w} \cdot C_{per\_w}$$

This formula produces the characteristic parabolic delay profile of distributed RC lines.

---

## Gate Input Capacitance (Fanout)

NAND gate inputs present capacitive load to driving wires, affecting both delay and energy consumption.

### Modeling Gate Capacitance

Each NAND input is modeled as an additional node connected to the wire with:

- An equivalent wire length $l_{socket}$ representing the input capacitance
- The capacitance $C_{gate} = l_{socket} \cdot C_{per\_w}$

This "virtual wire" approach naturally integrates gate loading into the Elmore delay calculation.

### Fanout Effects

When a wire drives multiple NAND inputs (fanout), the total downstream capacitance increases, resulting in:

1. **Increased delay**: More capacitance means longer RC delay
2. **Increased energy**: More capacitance means more switching energy

This models the real-world FO4 (fanout-of-4) delay metric used in VLSI design.

---

## Gate Activation and Wire Propagation

Signal changes involve two sequential phases:

### Phase 1: Gate Activation

When a NAND gate's inputs change, the gate evaluates and begins switching its output. The gate delay is:

$$T_{gate} = R_{gate} \cdot (C_{downstream} + \frac{C_{gate}}{2}) + T_{fixed}$$

Where:

- $R_{gate}$: Output resistance of the NAND gate
- $C_{downstream}$: Total capacitance of the driven wire network
- $C_{gate}$: Internal gate capacitance
- $T_{fixed}$: Fixed intrinsic gate delay

### Phase 2: Wire Propagation

After the gate switches, the signal propagates through the wire network according to the Elmore delays computed for each node.

Both phases consume energy and are visualized separately in the simulation.

---

## Fixed Gate Delay

Beyond the RC-dependent delay, each NAND gate has a fixed intrinsic delay ($T_{fixed}$) representing:

- Internal transistor switching time
- Parasitic capacitances not modeled explicitly

This prevents unrealistically fast gates when driving minimal loads and ensures a minimum switching time.

---

## Maximum Wire Delay

To maintain simulation stability and reasonable gameplay, wires have a maximum allowed delay:

$$T_{max} = 6000 \text{ ticks}$$

Wires exceeding this limit are flagged as errors ("too slow"). This encourages players to:

- Use buffer gates to break up long wires
- Route signals through faster upper metal layers
- Design compact, efficient layouts

The long wire constraint is also linked to the maximum event scheduling time allowed in the circular event queue.

---

## Energy Computation

The simulation tracks energy consumption for power optimization challenges.

### Switching Energy

Each wire transition consumes energy proportional to its total capacitance:

$$E_{pulse} = \frac{1}{2} C_{total} V_{dd}^2$$

Where $V_{dd}$ is the supply voltage. NAND activations also consume energy, with same formula but using only the gate capacitance:

$$E_{gate} = \frac{1}{2} C_{gate} V_{dd}^2$$

$$E_{total} = E_{gate}  + E_{pulse} $$ 

### Energy 

We've seen how much energy an event is supposed to consume, but we don't display them instantaneously, instead we spread it through time using an exponential decay approximation as follows.

Potential energy at time $t$ decays as:

$$E_k[t + 1] = (1- \gamma_k) E_k[t] $$

$$P_k[t] = \gamma_k E_k[t] $$

$$E[t]  = \sum_k E_k[t]$$

$$P[t]  = \sum_k P_k[t]$$

Where $P[t]$ is the power at time $t$. The "energy" shown to the user is the sum of the "deposited" power over time, while $E$ is the "potential" energy. 

The key idea is that when an event happens, we don't deposit the energy directly, we include them into some "potential energy bins" $E$, which are released with a decay factor, so it is spread through time. Different bins decay with different rates, and the way we spread the energy through bins will depend on the "delay time" of the event, then we just need to track each bin instead of tracking every single event.

The decay $\gamma_k$ is chosen so that after $T_k$ steps, only 1% of the initial energy is left. This is supposed to approximate a "decay in T steps".

$$ \gamma_k^{T_k} = 0.01 $$

Then we choose $T_k = 2^k$ as time bins, and when an event happens with time $T$ we spread its energy accross the two closest bins $ T_i < T < T_{i+1} $.

---

## Event-Driven Simulation

The simulation uses a **variable-delay event-driven** algorithm, similar to those used in commercial VLSI simulators.

**Reference**: Event-driven simulation is described in Mishra and Dutt, _Architecture Description Languages for Programmable Embedded Systems_, Chapter 6.

### Algorithm Overview

```
while simulation not idle:
    current_tick++

    // Process events scheduled for this tick
    for each event in queue[current_tick]:
        update socket value
        if socket belongs to NAND:
            activate NAND (set to PENDING state)

    // Update active NANDs
    for each active NAND:
        if state == PENDING:
            compute new output value
            if output changes:
                set activation counter
                state = RUNNING
        else if state == RUNNING:
            decrement counter
            if counter == 0:
                dispatch pulse to wire
                schedule events for all fanout sockets
                state = IDLE
```

---

## Circular Event Queue

The event queue uses a **circular buffer** for efficient O(1) event scheduling and retrieval.

### Structure

```
EventQueue:
    q[MAX_DELAY][]    // Array of event lists, one per tick
    cur_time          // Current position in circular buffer
    pending_events    // Total pending event count
```

### Operations

**Schedule event $\Delta t$ ticks in the future:**

```
index = (cur_time + dt) % MAX_QUEUE_DELAY
q[index].append(event)
pending_events++
```

**Advance time:**

```
process all events in q[cur_time]
q[cur_time].clear()
cur_time = (cur_time + 1) % MAX_QUEUE_DELAY
```

The maximum queue delay (8000 ticks) must exceed the maximum wire delay plus gate delay to prevent event wraparound.

---

## NAND Gate States

Each NAND gate can be in one of three states:

| State       | Description                                         |
| ----------- | --------------------------------------------------- |
| **IDLE**    | Gate is stable, not processing any change           |
| **PENDING** | Input changed, evaluating if output needs to change |
| **RUNNING** | Output is switching, counting down activation timer |

### State Transitions

```
IDLE -> PENDING:  When any input socket receives new value
PENDING -> RUNNING:  When computed output differs from current output
PENDING -> IDLE:  When computed output equals current output (no change)
RUNNING -> IDLE:  When activation counter reaches zero (output dispatched)
```

---

## Wire Pulses and Visualization

Wire states are encoded in a compact format for efficient GPU rendering.

### Pulse Encoding

Each wire's visual state is packed into a 32-bit integer:

```
bits [0-1]:   before_value  (0=off, 1=on, 2=undefined, 3=error)
bits [2-3]:   after_value
bits [4-31]:  pulse_tick    (time when transition started)
```

### Shader-Based Rendering

The GPU shader computes the visual state of each pixel:

1. Look up the wire ID for this pixel from the distance map
2. Fetch the pulse data for that wire
3. Compare `pulse_tick + pixel_delay` with `current_tick`
4. If current_tick is past the delay, show `after_value`; otherwise show `before_value`

This allows smooth animated propagation of signals along wires without CPU intervention.

---

## Time-Travel with Delta Patches

The simulation supports rewinding time through a **delta-based** (patch) system.

### Patch Structure

Each simulation tick produces a patch containing all state changes:

- Wire pulse changes (XOR-encoded)
- Socket value changes (XOR-encoded)
- NAND state changes
- Event queue modifications
- Energy accumulator updates

### Reversible Operations

All state changes use XOR encoding for perfect reversibility:

```
Forward:  new_state = old_state XOR patch
Backward: old_state = new_state XOR patch
```

### Undo/Redo Stacks

Two stacks store patches for time navigation:

```
Step Forward:
    if redo_stack not empty:
        patch = redo_stack.pop()
    else:
        patch = compute_new_patch()
    apply_forward(patch)
    undo_stack.push(patch)

Step Backward:
    patch = undo_stack.pop()
    apply_backward(patch)
    redo_stack.push(patch)
```

This enables frame-perfect rewinding for debugging circuits.

---

## Limitations

The simulation makes several simplifying assumptions:

1. **No inductance**: Wire inductance is ignored (valid for typical on-chip interconnect)
2. **Simplified PI model**: Distributed RC is approximated with lumped elements
3. **First-order delay**: Elmore delay is a first-order approximation; real waveforms have higher-order effects
4. **Uniform wire properties**: All pixels in a layer have identical R and C
5. **Cyclic graphs approximated**: Loops use spanning tree heuristic, not exact analysis

---

## References

1. W.C. Elmore, "The Transient Response of Damped Linear Networks with Particular Regard to Wideband Amplifiers," _Journal of Applied Physics_, vol. 19, pp. 55-63, January 1948.

2. J.M. Rabaey, A. Chandrakasan, and B. Nikolic, _Digital Integrated Circuits: A Design Perspective_, 2nd Edition, Prentice Hall, 2003.

3. R. Rubinstein, P. Penfield, and M.A. Horowitz, "Signal Delay in RC Tree Networks," _IEEE Transactions on Computer-Aided Design_, vol. CAD-2, no. 3, pp. 202-211, July 1983.

4. L.T. Pillage and R.A. Rohrer, "Asymptotic Waveform Evaluation for Timing Analysis," _IEEE Transactions on Computer-Aided Design_, vol. 9, no. 4, pp. 352-366, April 1990.

---

_This simulation engine aims to provide an educational and engaging experience while remaining faithful to the fundamental physics of integrated circuit timing. The techniques used are simplified versions of algorithms employed in professional EDA tools._
