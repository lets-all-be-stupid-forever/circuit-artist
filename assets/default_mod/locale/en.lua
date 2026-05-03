T = {}
T.custom_name = "Unsorted / Sandbox"
T.custom_desc = "Unsorted Levels."
T.sandbox_name = 'Sandbox'
T.sandbox_desc= [[
Sandbox mode with a clock that triggers when circuit is idle.

No objective.
]]
T.sandbox_inputs_title = "Inputs"
T.sandbox_inputs_text = [[
The `clock` input allows you to create synchronous circuits. It triggers `on` or `off` when the circuit gets idle (it won't update in a constant rate).

The `power_on_reset` input allows you to initialize memory when applicable. It stays on (`1`) for the 2 first clock cycles, then it becomes off (`0`) for the rest of the simulation.
]]

-- Basics Campaign

T.basics_name = "Tutorial I - Wires and Gates"
T.basics_desc = [[
Learn the fundamentals of digital circuits.

Start by connecting wires, then build your first logic gates from NAND - the universal building block.

By the end, you'll understand how all Boolean logic can be constructed from a single gate type.
]]

T.wires1_name = "Wires"
T.wires1_desc = [[
- Connect wire `A_in` to wire `A_out`.
]]

T.wires2_name = "Crossing Wires"
T.wires2_desc = [[
- Connect wire `A_in` to `A_out`
- Connect wire `B_in` to `B_out`
]]

T.wires3_name = "Wires I/O"
T.wires3_desc =[[
- Connect wire `A_in` to `A_out_1` and `A_out_2`.
- Connect wire `B_in` to `B_out_1` and `B_out_2`.
]]

T.wires4_name = "Multi-Bit Wires"
T.wires4_desc =[[
  - Connect the 4-bit wire `A_in` to `A_out`.
]]
T.wires4_tip_title = "Tip: Line Tool"
T.wires4_tip_text = [[
You can use the "line tool" to make the task easier. It allows you to draw multiple parallel wires at the same time. Please check the tool tooltip for more instructions.
]]
T.wires4_pin_title = "Pin bit order"
T.wires4_pin_text = [[
For multi-bit pins, bit0 is the uppermost wire, bit1 is the one below it, and so on, for both input and output pins.
]]

T.nand_name = "NAND Gate"
T.nand_desc = [[
  - Compute the NAND operation of the input pins `a` and `b`.
]]
T.nand_example_title = 'NAND Gate Example'
T.nand_picture = 'NAND Gate Picture'

T.not_name = "NOT Gate"
T.not_desc = [[
- Invert the value of the input pin `a`.
]]
T.not_picture = 'NOT Gate Picture'
T.not_schema='NOT Gate Schema'

T.and_name="AND Gate"
T.and_desc = [[
- Compute the AND operation of the input pins `a` and `b` and write it to `a_and_b` output pin.
]]
T.and_picture ='AND Gate Picture'
T.and_schema = 'AND Gate Schema'



T.or_name = "OR Gate"
T.or_desc = [[
- Compute the OR operation of the input pins `a` and `b` and write it to `a_or_b` output pin.
]]
T.or_picture = 'OR Gate Picture'
T.or_schema = 'OR Gate Schema'


T.xor_name="XOR Gate"
T.xor_desc=[[
- Compute the XOR operation of the input pins `a` and `b` and write it to `a_xor_b` output pin.
]]
T.xor_picture = 'XOR Gate Picture'
T.xor_schema = 'XOR Gate Schema'

T.a_eq_b1_name="A equals B"
T.a_eq_b1_desc=[[
Given two `1`-bit inputs `a` and `b`, return 1 if they're equal, or 0 if they're different.
]]

T.a_eq_b1_tip = 'tip'

T.a_eq_b2_name ="A equals B x32"
T.a_eq_b2_desc= [[
Given two `32`-bit inputs `a` and `b`, return 1 if they're equal, or 0 if they're different.
]]

T.a_eq_b2_tip = 'tip'
T.a_eq_b2_tip_text = 'A equals B if: `A0=B0` AND `A1=B1` AND `A2=B2` AND `A3=B3` ... AND `A31=B31`.'


T.sevseg_name = "Tutorial II - 7 Segment Display"
T.sevseg_desc = [[
Apply your logic gate knowledge to build a practical circuit.

Learn about decoders and how to convert binary numbers into visual output - the same principle used in calculators and digital clocks.
]]
T.match7_name= "7?"
T.match7_desc = [[
Set output to 1 if `n` is `7`, otherwise set it to 0.
]]

T.match7_or_23_name = "7 or 23?"
T.match7_or_23_desc = [[
- Set output to 1 if `n` is `7`.
- Set output to 1 if `n` is `23`.
- Set output to 0 otherwise.
]]

T.decoder1_name = "1-Bit Decoder"
T.decoder1_desc = [[
Implement a 1:2 decoder that takes a 1-bit input `a` and activates exactly one of the two output bits based on the input value:

- a=0 -> output[0]=1, output[1]=0 (binary: 10)
- a=1 -> output[0]=0, output[1]=1 (binary: 01)

]]

T.decoder2_name ="1-Bit Decoder with Enable"
T.decoder2_desc = [[

Implement a 1:2 decoder with enable.

The enable input is an additional control signal: when `enable=1` (ON), the decoder operates normally. When `enable=0` (OFF), all outputs are zero.

]]

T.decoder_2bit_name ="2-Bit Decoder"
T.decoder_2bit_desc =[[

Implement a 2:4 decoder that takes a 2-bit input `a` and activates exactly one of the 4 output bits based on the input value:

- a=0 -> output=0001
- a=1 -> output=0010
- a=2 -> output=0100
- a=3 -> output=1000

]]

T.decoder3_name ="5-Bit Decoder"
T.decoder3_desc = [[
Implement a 5:32 decoder.
]]

T.match_many_name ="Is it one of these?"
T.match_many_desc = [[
  Set output to 1 if `n` is one of these:

  - 1
  - 3
  - 5
  - 6
  - 15
  - 21
  - 24
  - 31

  Otherwise set it to 0.
]]

T.match_many_tip_title ="Tip"
T.match_many_tip_text ="You can pick the 5:32 decoder from the previous level and select only the outputs you're interested in."
T.match_many_table_title = "Table"
T.match_many_table_text = [[
This is a `truth table`, it shows you the desired output for each of the possible inputs.

| n  | output |
-----------
| 0  | 0 |
| 1  | `1` |
| 2  | 0 |
| 3  | `1` |
| 4  | 0 |
| 5  | `1` |
| 6  | `1` |
| 7  | 0 |
| 8  | 0 |
| 9  | 0 |
| 10 | 0 |
| 11 | 0 |
| 12 | 0 |
| 13 | 0 |
| 14 | 0 |
| 15 | `1` |
| 16 | 0 |
| 17 | 0 |
| 18 | 0 |
| 19 | 0 |
| 20 | 0 |
| 21 | `1` |
| 22 | 0 |
| 23 | 0 |
| 24 | `1` |
| 25 | 0 |
| 26 | 0 |
| 27 | 0 |
| 28 | 0 |
| 29 | 0 |
| 30 | 0 |
| 31 | `1` |
---------
]]
T.sevenseg_name = "Seven Segment Display"
T.sevenseg_desc = [[
Given a 4-bit input `n`, display it in a 7-segment display as in the image below.
]]
T.sevenseg_sample = 'Seven Segments'

T.routing1_name = "Tutorial III - Routing Bits"
T.routing1_desc = [[
Master data flow control with multiplexers and demultiplexers.

These components let you select, route, and broadcast signals - essential building blocks for CPUs, memory systems, and communication buses.
]]

T.mux_2_1_name = "Multiplexer 2:1"
T.mux_2_1_desc = [[
Create a "1-bit selector" component, where the result is:
- `a` when `s` is `0`
- `b` when `s` is `1`
]]
T.mux_2_1_schema = "MUX Schema"
T.mux_2_1_impl = "MUX Sample Implementation"
T.mux_2_1_analogy = "MUX Analogy to Switches"

T.mux_4_1_name = "Multiplexer 4:1"
T.mux_4_1_desc = [[
Create a "selector" component, where the result is:
- `a` when `s` is `0`
- `b` when `s` is `1`
- `c` when `s` is `2`
- `d` when `s` is `3`
]]

T.mux_4_1_larger = "Larger Multiplexers"
T.mux_4_1_larger_text = [[
You can build bigger multiplexers by combining smaller ones.
]]
T.mux_4_1_schema= "4:1 MUX Schema"

T.mux_4_2_name = "Multiplexer 4:2"
T.mux_4_2_desc = [[
Create a "selector" component, where the result is:
- `a` when `s` is `0`
- `b` when `s` is `1`
]]

T.bus2_name = "2-Bit Data Bus"
T.bus2_desc = [[
You have 4 external "chips" C0, C1, C2 and C3, and each of the chips have input and output wires.

Each chip reads data from C_in and writes data to C_out.

The thing is, they need to communicate with each other. One way they can do that is by sending the output from `one` of those chips to all the others, then they can alternate who speaks at a given time.

For that, there will be a 2-bit selector signal `s` telling which chip will be talking, and your task is to send the `OUTPUT` data from that chip to the `INPUT` of all other chips (including the origin itself).

In other words, create a 4:1 MUX that broadcasts the selected chip's output to all chips' inputs.

]]

T.bus2_example = 'Example'
T.bus2_example_text = [[
Let's say we have:
- c0_out=0
- c1_out=1
- c2_out=2
- c3_out=3

Then, when `s=1`, we want to have:

- c0_in=1 `(c1_out)`
- c1_in=1 `(c1_out)`
- c2_in=1 `(c1_out)`
- c3_in=1 `(c1_out)`
]]
T.bus2_example2 = 'Example II'

T.demux_1_2_name = "Demux 1:2"
T.demux_1_2_desc = [[
Create a 1:2 "router" component:
- When `s=0` -> `d0=a`, `d1=0`
- When `s=1` -> `d0=0`, `d1=a`
]]

T.demux_1_2_analogy_title ='Analogy with a MUX'
T.demux_1_2_analogy_text = [[A DEMUX is like a reversed MUX:
- MUX: N inputs -> 1 output (selector picks which input)
- DEMUX: 1 input -> N outputs (selector picks which output receives it)
]]
T.demux_1_2_analogy2_title ='Analogy with a Decoder'
T.demux_1_2_analogy2_text = "When the data input to the DEMUX equals 1, we have a regular decoder (where the 'selector' becomes the decoder's input)."
T.demux_1_2_demux = '1:2 Demux'
T.demux_1_2_demux2 = '1:2 Demux Example'
T.demux_1_2_analogy3 = '1:2 Demux Analogy'

T.demux_1_4_name = "Demux 1:4"
T.demux_1_4_desc = [[
  Create a 1:4 "router" component:
  - When `s=0` -> `d0=a` d1=0 d2=0 d3=0
  - When `s=1` -> d0=0 `d1=a` d2=0 d3=0
  - When `s=2` -> d0=0 d1=0 `d2=a` d3=0
  - When `s=3` -> d0=0 d1=0 d2=0 `d3=a`
]]
T.demux_1_4_tip_title = '1:4 Demux Tip'

T.demux_2_4_name = "Demux 2:4"
T.demux_2_4_desc = [[
  Create a 2:4 "router" component:
  - When `s=0` -> `d0=a` d1=0
  - When `s=1` -> d0=0 `d1=a`
]]

T.router_name = "2-Bit Data Router"
T.router_desc = [[
  You have 4 external "chips" C0, C1, C2 and C3, and each of the chips have input and output wires.

  Each chip reads data from C_in and writes data to C_out.

  The task is to send data from one chip to another chip. There will be 2 selectors: one for origin and one for destination, then your task is to send the output of the origin to the input of the destination, all other chips should receive 0.

  In other words, combine a MUX (to select source) and a DEMUX (to route to destination).
]]
T.router_example_title = 'Example'
T.router_example_text = [[
      Let's say we have:
      - c0_out=0
      - c1_out=1
      - c2_out=2
      - c3_out=3

      Then, when `origin=3` and `destination= 1`, we want to have:

      - c0_in=0
      - c1_in=3 `(c3_out)`
      - c2_in=0
      - c3_in=0
]]
T.router_example2_title = 'Example II'

-- Memory Campaign

T.memory1_name = "Tutorial IV - Memory"
T.memory1_desc = [[
Discover how circuits can remember.

So far, outputs have depended only on current inputs. In this tutorial, you'll learn to build circuits that can store information and change behavior over time.

Start with simple latches, progress to flip-flops and registers, and finish by building the NAND Processing Unit Prototype 0.
]]

T.latch_door_name = "SR Latch"
T.latch_door_desc = [[
Build an SR Latch - a circuit that allows you to set a stored bit to 0 or 1.

It should have an output bit `Q` that is "remembered", and two inputs:

- A set bit `S` that changes `Q` to 1
- A reset bit `R` that changes `Q` to 0.

Whenever `S` and `R` are both 0, the value of `Q`, the "stored" bit, should remain the same.

The SR latch is the building block for more sofisticated memory circuits.

We will ignore the case where `S`=1 and `R`=1 simulatenously (they're considered invalid states).

]]
T.latch_door_think_title = "How to build it"
T.latch_door_think_text = [[
Try connecting two NOT gates in a loop: the output of the first feeds into the second, and the output of the second feeds back into the first. Click on one of the wires to change its value - the circuit stabilizes with one wire at 0 and the other at 1. This is memory! But there's a problem: you can only change it by clicking, not via an input signal.

Now replace those NOT gates with NAND gates. Remember that NAND(x, 1) = NOT(x), so when one input is held at 1, a NAND behaves like NOT. But when you set that input to 0, the NAND outputs 1 regardless - this lets you "force" a wire to 1 via an input.

With two NAND gates in a loop, each with an extra input, you have a controllable memory cell: one input forces the output to 1, the other forces it to 0, and when both inputs are 1, the circuit remembers its previous state.
]]

T.dlatch_name = "D Latch"
T.dlatch_desc = [[
Create a memory circuit with 2 inputs `D` and `E` that stores a `Q` bit such that:

- When `E`=1: `Q` <- `D`
- When `E`=0: `Q` doesn't change.

Ie, it stores the value of `D` (data bit) whenever `E` (enable bit) is on.

]]
T.dlatch_example_title = "Example"
T.dlatch_example_text = [[
Example:
- 1st input: D=0 E=1 --> Q=0
- 2nd input: D=1 E=1 --> Q=1
- 3rd input: D=0 E=0 --> Q=1 (Q doesn't change because E=0)
- 4th input: D=1 E=0 --> Q=1 (Q doesn't change because E=0)
- 5th input: D=0 E=1 --> Q=0 (Q changes again because E=1)
]]
T.dlatch_symbol_title = 'Symbol'
T.dlatch_truthtable_title = 'Truth Table'
T.dlatch_impl_title = 'D Latch Implementation'

T.photo_name = "D Flip-Flop"
T.photo_desc = [[

Build a D Flip-Flop - an extension of the D-latch that only updates `Q` at one specific point in time instead of continuously.

The idea is that, instead of changing the `Q` value whenever D changes, there will be an extra input (`CLK`), and we only want the `Q` value to update when the `CLK` signal changes from 0 to 1. It's an auxiliary signal to tell the exact moment we want the memory to be updated.

You can check the wiki for more details.
]]
T.photo_analogy_title = 'The Camera Analogy'
T.photo_analogy_text = [[
Imagine a camera with a sensor that continuously reads a value, and a shutter button. When you press the button, the camera captures the sensor value at that exact moment - not before, not after. Even if the sensor changes while you hold the button, the photo doesn't change.

This is exactly what a D Flip-Flop does: it "photographs" the value of `D` at the exact moment `CLK` rises from 0 to 1.
]]
T.combo_detector_name = "Combo Detector"
T.combo_detector_desc = [[
You're given as input the "press state" of 2 buttons: button A and button B.

Whenever a button is pressed, the press state becomes 1 and then becomes 0 when it is released.
Only one button can be pressed at a time (never both). When no button is pressed, both inputs are 0.

Your task is to create a circuit that detects whenever the button A was pressed 4 times in a row. So return `combo_detected` as 1 if the last 4 button presses were A or 0 otherwise.

A button is considered as pressed when it is down (ie, input is 1).

The first 4 outputs are not checked (the system needs 4 presses to have enough history).

For example, let's say the press order is the following:

- B pressed --> combo=0
- A pressed --> combo=0
- A pressed --> combo=0
- B pressed --> combo=0
- A pressed --> combo=0
- A pressed --> combo=0
- A pressed --> combo=0
- A pressed --> `combo=1` <-- Detected!

]]
T.combo_detector_clock_title = 'Creating a Clock Signal'
T.combo_detector_clock_text = [[
Unlike previous levels, this level has no external clock input. Instead, you need to create your own clock signal from the button inputs.

A "button press" happens when either button goes from 0 to 1. You can detect this with:

`clock = button_A OR button_B`

This clock ticks whenever any button is pressed.
]]
T.combo_detector_timing_title = 'The Timing Problem'
T.combo_detector_timing_text = [[
If you derive the clock from the buttons and connect it directly to your flip-flops, you'll have a problem: the clock and the data (which button was pressed) arrive at the same time!

For a flip-flop to work correctly, the data input must be stable BEFORE the clock rising edge arrives. When both come from the same source, there's a race condition.
]]
T.combo_detector_delay_title = 'Clock Delay Solution'
T.combo_detector_delay_text = [[
The solution is to delay the clock signal so it arrives AFTER the data is stable.

You can add delay using a chain of NOT gates: `NOT(NOT(signal))` gives you the same value but delayed by 2 gate updates.

So your circuit should:
1. Use `button_A` directly as the data to store (1 if A was pressed, 0 if B was pressed)
2. Create the clock as `button_A OR button_B`
3. Delay the clock through NOT-NOT chains before sending it to your flip-flops
]]
T.combo_detector_history_title = 'Storing History'
T.combo_detector_history_text = [[
You need to remember the last 4 button presses. Use 4 flip-flops chained together:
- FF1 stores the most recent press
- FF2 stores the press before that
- FF3 stores the one before that
- FF4 stores the oldest of the 4

On each clock tick, shift the values: FF4 <- FF3 <- FF2 <- FF1 <- new_input
]]
T.combo_detector_output_title = 'Computing the Output'
T.combo_detector_output_text = [[
Once you have the last 4 presses stored, the output is simple:

`combo_detected = FF1 AND FF2 AND FF3 AND FF4`

If all 4 stored values are 1 (meaning all 4 presses were button A), output 1. Otherwise output 0.
]]

T.dflipflop_with_enable_name = "D Flip-Flop With Enable"
T.dflipflop_with_enable_desc = [[
Create a D Flip-Flop with enable (`E`). The enable tells the flip-flop to NOT store `D` when the clock reaches the rising edge.

When `E`=1, the flip flop should work normally, updating its stored value.

When `E`=0, the flip flop should ignore the input (by updating with its previous value instead).
]]
T.dflipflop_with_enable_hint_title = 'Hint'
T.dflipflop_with_enable_hint_text = [[
In the flip flop input, you can use the `E` signal to select between the flip flop's output (`Q`) and the `D` input.
]]
T.dflipflop_with_enable_example_title = 'Example'
T.dflipflop_with_enable_example_text = [[
- D=0 E=1 CLK=rising(0 to 1) --> Q =0
- D=1 E=1 CLK=rising(0 to 1) --> Q =1
- D=0 E=1 CLK=rising(0 to 1) --> Q =0
- D=0 E=0 CLK=rising(0 to 1) --> Q =0
- D=1 E=0 CLK=rising(0 to 1) --> Q =0 (no change)
- D=0 E=0 CLK=rising(0 to 1) --> Q =0
- D=1 E=1 CLK=rising(0 to 1) --> Q =1 (change)
]]
T.dflipflop_with_enable_tests_title = 'Test Cases'
T.dflipflop_with_enable_tests_text = [[
  CLK D E Q
#00 1 0 1 2
#01 0 0 1 2
#02 1 0 1 0
#03 1 0 1 0
#04 0 1 1 0
#05 1 1 1 1
#06 0 0 0 1
#07 1 0 0 1
#08 0 1 0 1
#09 1 1 0 1
#10 0 0 1 1
#11 1 0 1 0
#12 0 1 1 0
#13 1 1 1 1
#14 0 0 1 1
#15 1 0 1 0
#16 0 0 0 0
#17 1 0 0 0
#18 0 1 0 0
#19 1 1 0 0
#20 0 1 1 0
#21 1 1 1 1
#22 0 0 0 1
]]
T.dflipflop_with_enable_comment_title = 'Comment'
T.dflipflop_with_enable_comment_text = [[
The Enable signal is important for when you only want to update a few of your flip flops. For example, if you store a "memory" of 100 bits and just want to update a few you set most of them to E=0 and the one you want to change to E=1.
]]
T.dflipflop_with_enable_schema_title = 'Schema'

T.dff_w_r_name = "D FF With Reset"
T.dff_w_r_desc = [[
Create a D Flip-Flop with enable (`E`) and reset (`RST`). When the reset is on and the clock reaches rising edge (0->1), the value of the flip flop (`Q`) should become 0, independent of the value of `D` and `E`.

When `RST`=0, the flip flop should work normally (as a FF with enable).

When `RST`=1, the FF value should be set to 0.
]]
T.dff_w_r_hint_title = 'Hint'
T.dff_w_r_hint_text = [[
In the same place where the `E` signal chooses between `D` and `Q`, the `RST` signal can choose between those and `0` as input to the D-Flip Flop.
]]
T.dff_w_r_comment_title = 'Comment'
T.dff_w_r_comment_text = [[
The Reset signal is important at startup for example to initialize all your "data" to zero.
]]
T.dff_w_r_schema_title = 'Schema'
T.dff_w_r_tests_title = 'Test Cases'
T.dff_w_r_tests_text = [[
  CLK D E RST Q
#00 1 0 1 0 2
#01 0 0 1 0 2
#02 1 0 1 0 0
#03 1 0 1 0 0
#04 0 1 1 0 0
#05 1 1 1 0 1
#06 0 0 0 0 1
#07 1 0 0 0 1
#08 0 1 0 1 1
#09 1 1 0 1 0 Reset
#10 0 0 1 0 0
#11 1 0 1 0 0
#12 0 1 1 0 0
#13 1 1 1 0 1
#14 0 0 1 0 1
#15 1 0 1 0 0
#16 0 0 0 1 0
#17 1 0 0 1 0 Reset
#18 0 1 0 1 0
#19 1 1 0 1 0 Reset
#20 0 1 1 0 0
#21 1 1 1 0 1
#22 0 0 0 1 1
#23 1 1 1 1 0 Reset
#24 0 1 1 0 0
#25 1 1 1 0 1
#26 0 0 0 1 1
#27 1 0 0 1 0 Reset
#28 0 0 0 0 0
]]

T.register4_name = "4-bit Register"
T.register4_desc = [[
Create a 4-bit register with Enable and Reset.

A Register is simply a group of n flip-flops sharing the same clock, and they update simultaneously.

**Inputs:**
- `CLK`: Clock signal
- `Data`: 4-bit input value
- `E`: Enable (when 1, register updates; when 0, register keeps its value)
- `RST`: Reset (when 1, register resets to 0000, regardless of E)

**Output:**
- `Q`: 4-bit stored value

Reset takes priority over Enable: if `RST`=1, the register resets to 0 even if `E`=0.
]]
T.register4_hint_title = 'Hint'
T.register4_hint_text = [[
Use 4 D Flip-Flops with Enable and Reset. Connect the same CLK, E, and RST signals to all 4 flip-flops. Each flip-flop handles one bit of Data and produces one bit of Q.
]]
T.register4_tests_title = 'Test Cases'
T.register4_tests_text = [[
  CLK Data E RST Q
#00 1 0000 1 1 2222
#01 0 1010 1 1 2222
#02 1 1010 1 1 0000 E=1 RST=1
#03 0 1001 1 0 0000
#04 1 1001 1 0 1001 E=1 RST=0
#05 0 1100 0 0 1001
#06 1 1100 0 0 1001 E=0 RST=0
#07 0 0011 0 1 1001
#08 1 0011 0 1 0000 E=0 RST=1
#09 0 0110 1 0 0000
#10 1 0110 1 0 0110
#11 0 1111 1 0 0110
#12 1 1111 1 0 1111
]]

T.registerfile_name = "Register File 4 x 1-bit"
T.registerfile_desc = [[
Create a 4 x 1-bit register file.

A register file is a small, fast memory that holds multiple registers. Unlike a single register where you always write to the same place, a register file lets you choose which register to write to using an address.

**Inputs:**
- `rst`: Reset (sets all registers to 0 on rising edge)
- `clk`: Clock signal
- `data`: 1-bit value to write
- `waddr`: 2-bit write address (selects which register to write: 0, 1, 2, or 3)
- `write_enable`: When 1, writes `data` to the register selected by `waddr` on rising edge

**Outputs:**
- `y0`: Current value of register 0
- `y1`: Current value of register 1
- `y2`: Current value of register 2
- `y3`: Current value of register 3

All 4 register values are always visible at the outputs. The `waddr` only controls which one gets updated when writing.
]]
T.registerfile_hint_title = 'Hint'
T.registerfile_hint_text = [[
Use 4 D Flip-Flops with Enable and Reset - one for each register. The key is routing the `write_enable` signal to only the selected flip-flop.

Think about it: you need to enable exactly one of the 4 flip-flops based on `waddr`. A demultiplexer (demux) does exactly this - it routes a single input to one of several outputs based on a selector.

Use a 1:4 demux to route `write_enable` to the correct flip-flop's enable input.
]]
T.registerfile_tests_title = 'Test Cases'
T.registerfile_tests_text = [[
  rst clk data waddr we -> y0 y1 y2 y3
#00 1   0   0    00   0     ?  ?  ?  ? Power On
#01 1   1   0    00   0     0  0  0  0 Reset
#02 0   0   1    00   1     0  0  0  0
#03 0   1   1    00   1     1  0  0  0 y0 <-- 1
#04 0   0   1    01   1     1  0  0  0
#05 0   1   1    01   1     1  1  0  0 y1 <-- 1
#06 0   0   0    00   1     1  1  0  0
#07 0   1   0    00   1     0  1  0  0 y0 <-- 0
#08 0   0   1    11   1     0  1  0  0
#09 0   1   1    11   1     0  1  0  1 y3 <-- 1
#10 0   0   1    10   0     0  1  0  1
#11 0   1   1    10   0     0  1  0  1 no op (we=0)
]]

T.npu1_name = "NPU-P0"
T.npu1_desc = [[
Create the NAND Processing Unit Prototype 0 (NPU-P0).

Until now, circuits performed a fixed computation: given the same inputs, they always do the same thing. The NPU-P0 is different. It reads an operation code (`op`) each clock cycle and performs the corresponding action. By feeding it a sequence of operations, you can make it compute things that no fixed circuit could achieve.

The NPU-P0 should have 4 registers (R0, R1, R2, R3) for storage and should support 4 operations. The `op` input selects which operation to perform, while other inputs specify parameters like which register to target or what value to load.

`Inputs:`
- `rst`: Reset signal (should set all registers to 0 on rising edge)
- `clk`: Clock signal (operations should execute on rising edge)
- `op`: 2-bit operation code
- `data`: 1-bit value to load directly into a register (used by LOAD)
- `reg1`: 2-bit register selector (destination)
- `reg2`: 2-bit register selector (source)

`Operations (should be active on clock rising edge):`
- `op=0` NOP: Do nothing
- `op=1` LOAD: Load the `data` bit into register `reg1`
- `op=2` MOV: Copy the value of register `reg2` into register `reg1`
- `op=3` NAND: Compute R0 NAND R1 and store result in R0

`Output:`
- `y`: Should output the current value of R3
]]
T.npu1_data_title = "Note on 'data' input"
T.npu1_data_text = [[
The `data` input provides a constant value that gets loaded directly into a register. In CPU terminology, this kind of constant is often called an "immediate" value, because the value is immediately available in the instruction itself, rather than being fetched from a register or memory.
]]
T.npu1_asm_title = "NPU-P0 Assembly Reference"
T.npu1_asm_text = [[
The NPU-P0 understands 4 instructions:

`NOP` (op=0)
Do nothing. Registers remain unchanged.

!hl

`LOAD Rx, value` (op=1, reg1=Rx data=value)
Load a constant (0 or 1) into register Rx.

Examples:
`LOAD R0, 1` --> R0 <- 1
`LOAD R2, 0` --> R2 <- 0

!hl

`MOV Rx, Ry` (op=2, reg1=Rx, reg2=Ry)
Copy the value from register Ry into register Rx.

Examples:
`MOV R1, R0` --> R1 <- R0
`MOV R3, R2` --> R3 <- R2

!hl

`NAND` (op=3)
  Compute R0 NAND R1 and store the result in R0.
  Basically: R0 <- NAND(R0,R1)

]]
T.npu1_prog_title = "Test Program: AND.asm"
T.npu1_prog_text = [[
; =============================
; AND.asm
; Platform: NPU-P0
; Computes R3 = A AND B
; Input: A, B (constants)
; Output: R3
; =============================

`LOAD R0, A`       ; R0 = A
`LOAD R1, B`       ; R1 = B
`MOV R2, R0`       ; R2 = A (backup)
`MOV R3, R1`       ; R3 = B (backup)
`NAND`             ; R0 = A NAND B
`MOV R1, R0`       ; R1 = A NAND B
`NAND`             ; R0 = (A NAND B) NAND (A NAND B) = A AND B
`MOV R3, R0`       ; R3 = result

; The test runs this program 4 times with:
; A=0 B=0 -> R3=0
; A=0 B=1 -> R3=0
; A=1 B=0 -> R3=0
; A=1 B=1 -> R3=1
]]


T.wiki_tldr_title = "TLDR"
T.wiki_hotkeys_name = "Hotkeys"
T.wiki_hotkeys_text = [[

`HOTKEYS`

`Undo`
Ctrl-Z

`Redo`
Ctrl-Shift-Z
Ctrl-Y

`Navigation`
W = Up
A = Left
S = Down
D = Right
= (equal) zoom in
- (minus) zoom out

]]

T.wiki_wires1_name = "Basic wires"
T.wiki_wires1_text = [[

`BASIC WIRES`

!img:wiki/imgs/wires_pins_example.png

Wires are formed by connected non-black pixels. Wires can have 3 states as displayed below:
- `ON` (lighter color)
- `OFF` (darker color)
- `UNDEFINED` (magenta color)
!img:wiki/imgs/wires1.png
The `ON` state represents the logic `1`, and the `OFF` state represents the logic `0`. Wires state can be toggled (1->0 or 0->1) by clicking on it during simulation mode.
!img:wiki/imgs/wires2.png
The `UNDEFINED` state means that we don't know the state of the wire. Most wires start simulation in this state.
]]

T.wiki_wires_crossing_name =  "Crossing Wires"
T.wiki_wires_crossing_text = [[

`CROSSING WIRES`

The width or shape of a wire is not important, nor its color, as long as the pixels are connected.
!img:wiki/imgs/wires3.png
`Wires are allowed to cross`. Wires only connect at T or L connections.
!img:wiki/imgs/wire_cross.png
Wires are only connected via up/down/left/right pixel neighbours. Diagonal pixels are not considered connected. In the example below, in the first case you have 4 different wires, while in the second example you have a single wire.
!img:wiki/imgs/wire_diagonal.png

]]

T.wiki_wires_io_name = "Wires IO"
T.wiki_wires_io_text = [[

`WIRE INPUT AND OUTPUT`

`Each wire can have only 1 or 0 inputs`. If you connect a wire to more than 1 input, an error message will be displayed and the simulation will not start.

If a wire has no input, it will be automatically initialized with a 0 value at the beginning of the simulation.


]]

T.wiki_bit_order_name = "Bit Order"
T.wiki_bit_order_text = [[

`BIT ORDER`

Multi-wire "connections" can be used to represent numbers. In that case, for input/output connections, the game uses the `top-most wire as least significant bit`.

In your chip, you can use your own conventions for the least significant bit in vertical and horizontal multi-bit wires. However, the line tool corner (shift/control keys) mode works best if you keep the top-most/left-most as the least significant bit convention.

!img:wiki/imgs/bit_order.png

]]

T.wiki_posint_name =  "Number Representation"
T.wiki_posint_text = [[

`NUMBERS IN BINARY`

In everyday life, we count using the decimal system (base-10), which uses ten different digits: 0, 1, 2, 3, 4, 5, 6, 7, 8, and 9.

In computers and digital circuits, we use the binary system (base-2), which uses only TWO digits: 0 and 1.

How Binary Numbers Work:

Just like in decimal where each position represents a power of 10 (ones, tens, hundreds, thousands...), in binary each position represents a power of 2.

Reading from RIGHT to LEFT:
- The rightmost bit (bit 0) represents 2^0 = 1
- The next bit (bit 1) represents 2^1 = 2
- The next bit (bit 2) represents 2^2 = 4
- The next bit (bit 3) represents 2^3 = 8
- And so on: 16, 32, 64, 128, 256...

To find the decimal value of a binary number, add up the powers of 2 for each position that has a 1.

Examples:

Binary: 101
= 1×4 + 0×2 + 1×1
= 4 + 0 + 1
= 5 in decimal

Binary: 1010
= 1×8 + 0×4 + 1×2 + 0×1
= 8 + 0 + 2 + 0
= 10 in decimal

Binary: 11111
= 1×16 + 1×8 + 1×4 + 1×2 + 1×1
= 16 + 8 + 4 + 2 + 1
= 31 in decimal

Binary: 00000 = 0 in decimal
Binary: 00001 = 1 in decimal
Binary: 00010 = 2 in decimal
Binary: 00011 = 3 in decimal
Binary: 00100 = 4 in decimal
Binary: 00101 = 5 in decimal

This is how digital circuits represent numbers using only electrical signals that are either ON (1) or OFF (0).
]]

T.wiki_customlevel_name = "Custom Level"
T.wiki_customlevel_text = [[

`CUSTOM LEVEL`

You can write custom levels using `local` lua script.

The `examples/` folder in the game's directory contains a few examples.

The script needs to define 4 functions:

- `_Setup()`: Declares ports/pins. (called once on load)
- `_Start()`: Initializes simulation variables. (called before each simulation)
- `_Update()`: Level logic. Called every N ticks or whenever simulation "stops".
- `_Draw()`: Draws on screen. Called every UI frame during simulation.

More details:

!hl

`_Setup()`: This function should define the ports of the level. It is called right after the lua scripts are loaded. Each port is defined by: input or output, the number of wires and its name.

Input (as in input to the script), is defined by `AddPortIn(numWires, wireName)` and output wires as `AddPortOut(numWires, wireName)`. These functions return the ID of the port, which should be later used to read/write to it.

Example:

function _Setup()
  PORT_A = AddPortIn(2, 'a', LEFT)
  PORT_B = AddPortOut(4, 'b', RIGHT)
end

!hl

`_Start()`: Called when simulation starts, before any update. It should initialize simulation data and can declare some simulation parameters.

- `SetUpdateInterval(N)` rate in which _Update() is called. if N=0, it is called whenever circuit stops. If N>0, _Update() is called every N ticks.

A fixed tick rate might be useful for CPU or fixed clock design, but is sensitive to critical path design.


- `SetBaseTPS(T)`: Simulation's Ticks Per Second (TPS) for speed 3.

Default value is 240.

The scaling TPS per speed is:

- speed1: BaseTPS / 16
- speed2: BaseTPS / 4
- speed3: BaseTPS
- speed4: BaseTPS * 4
- speed5: BaseTPS * 32
- speed6: BaseTPS * 128

It allows higher simulation speeds, might be useful for CPU designs that need higher TPS.

!hl

`_Update()`: This function should contain the evolution logic of the level. It can read or write to ports using the `value = ReadPort(portId)` or `WritePort(portId, value)`.

Example:

function _Update()
  local a = ReadPort(PORT_A)
  WritePort(PORT_B, a+1)
end

!hl

`_Draw()`: Called every drawing frame, it `should not write to ports` here (it can read).

Implemented functions are: (many of them are based on raylib's implementation)

- `DrawRectangle(x, y, width, height, {r, g, b, a})` (colors are defined by {r,g,b,a} array with 0-255 range)
- `DrawText(txt, x, y, {r,g,b,a})` Draws text on screen.
- `MeasureText(txt)` returns the length in pixels of the text (when calling DrawText).
- `DrawTextBox(txt, x, y, w, {r,g,b,a})` Same as DrawText but will jump line when length goes past w (good for long text)
- `rlPushMatrix()` pushes (saves) matrix for transformations
- `rlPopMatrix()` pops (reverts) matrix for transformations
- `rlScalef(sx, sy, sz)` scales drawing (mind putting sz to 1)
- `rlTranslatef(tx, ty, tz)` translates drawing (mind putting tz to 0)

You can check raylib documentation/examples for more info/or more ideas on how to extend API.

]]

T.wiki_nand_name = "Nand Gate"
T.wiki_nand_text = [[

`NAND GATE`

A NAND logic gate is represented by 3 pixels, as shown below.

!img:wiki/imgs/nand0.png

The NAND logic gate has 2 inputs and 1 output. It reads the values of the inputs and, depending on those values, assigns a value to the output wire. The assignment table can be found below:

 a=0 b=0 --> NAND=1
 a=0 b=1 --> NAND=1
 a=1 b=0 --> NAND=1
 a=1 b=1 --> NAND=0

For example, if the 2 inputs are 0, then the output will be assigned to 1. See examples below.

!img:wiki/imgs/nand2.png

The NAND gates can be drawn in any orientation (facing up, down, left or right):

!img:wiki/imgs/nand3.png

Every non-black pixel in the image that is not part of a NAND gate is considered a wire, and has some state associated to it, as described below.

]]

T.wiki_not_name = "Not Gate"
T.wiki_not_text = [[

`NOT GATE`

The not gate inverts the value of an input bit A.

Example:
a=0 --> NOT=1
a=1 --> NOT=0
!img:wiki/imgs/not2.png
A not gate can be create using a single NAND gate, as shown in the picture above.

]]

T.wiki_and_name = "And Gate"
T.wiki_and_text= [[

`AND GATE`

The AND gate returns a 1 only if both inputs are 1, otherwise returns a 0. I.e., the output is only 1 if `a` AND `b` are 1.

Example:
a=0 b=0 --> AND=0
a=0 b=1 --> AND=0
a=1 b=0 --> AND=0
a=1 b=1 --> AND=1
!img:wiki/imgs/and2.png
An AND gate can be created using 2 NAND gates, as shown below. That is equivalent to a NAND gate followed by a NOT gate.

]]

T.wiki_or_name = "Or Gate"
T.wiki_or_text = [[

`OR GATE`

The OR gate returns a 1 if either input `a` OR input `b` is 1.

Example:
a=0 b=0 --> OR=0
a=0 b=1 --> OR=1
a=1 b=0 --> OR=1
a=1 b=1 --> OR=1
!img:wiki/imgs/or2.png
An OR gate can be created using 3 NAND gates.

]]

T.wiki_xor_name = "Xor Gate"
T.wiki_xor_text = [[

`XOR GATE`

The XOR gate returns 1 if exactly one of the inputs is 1; otherwise, it returns 0. (It's like a bit sum without carry.)

Example:
a=0 b=0 --> XOR=0
a=0 b=1 --> XOR=1
a=1 b=0 --> XOR=1
a=1 b=1 --> XOR=0
!img:wiki/imgs/xor2.png
A XOR gate can be created using 4 NAND gates.

]]

T.wiki_decoder1_name = "Decoder"
T.wiki_decoder1_text = [[

`DECODER`

!img:wiki/imgs/decoder_tut1.png

A decoder takes a binary input and outputs a signal where exactly one bit is high, indicating which input value was received.
Example:
A=0 --> D=01 (bit 0 active)
A=1 --> D=10 (bit 1 active)

!img:wiki/imgs/decoder_tut2.png

A 1-bit input decoder can be implemented as in the example above.

]]

T.wiki_mux_name = "Mux"
T.wiki_mux_text = [[

`MUX`

!img:wiki/imgs/mux1.png

A `MUX` is like a data `selector`: Given 2 inputs `a` and `b`, and a selection input `s`, if `s` is 0, `MUX` returns the value of `a`, and if `s` is 1, `MUX` returns the value of `b`.

!img:wiki/imgs/mux3.png

Muxes work like selectors (or a "switch"). They're useful for example when you have multiple calculations and want to select only one of the results. It's like an 'if' statement in programming: IF S=0, RETURN A; ELSE IF S=1, RETURN B.

!img:wiki/imgs/mux2.png
]]

T.wiki_demux_name = "Demux"
T.wiki_demux_text = [[

`DEMUX`

!img:wiki/imgs/demux1.png

A `DEMUX` is like a bit `router`: Given an input bit `a`, 2 outputs `Y0` and `Y1` and a selector bit `s`, if `s` is 0, then set `Y0` to the value of `a` and `Y1` to 0. Otherwise, if `s` is 1, set `Y0` to 0 and `Y1` to the value of `a`.
!img:wiki/imgs/demux3.png
Demuxes work like routers: you have a wire and the selector bit chooses to which wire the input bit should go.
Example:
a=0, s=0 --> Y0 = 0, Y1 = 0
a=1, s=0 --> Y0 = 1, Y1 = 0
a=0, s=1 --> Y0 = 0, Y1 = 0
a=1, s=1 --> Y0 = 0, Y1 = 1
A DEMUX can be implemented as follows:
!img:wiki/imgs/demux2.png
You can also create DEMUXes with more outputs by combining DEMUXes with fewer outputs:
!img:levels/imgs/demux4.png
]]

T.wiki_srlatch_name ="SR Latch"
T.wiki_srlatch_text = [[

`SR Latch`

This section will show one way to store 1 bit of memory.

It's generally not possible to store memory without using cycles, ie, wires that have outputs interlinked with some inputs somehow. We generally want to avoid arbitrarily using cycles in circuits because they can make the circuit complicated and can introduce oscillation errors, but bit storage is an exception.

The `SR latch` is a simple interlinked configuration of gates that is stable and can "remember" its previous configuration. You can change its values using the right inputs:
!img:wiki/imgs/mem1.png
The `S` and `R` inputs stand for `set` and `reset`: you can see that when S=1 and R=0, we have the value of `Q` updated to 1, and when R=1 and S=0, the value of `Q` is reset to 0. When inputs are 0,0 the latch doesn't do anything, it just maintains the previous set result, thus working as a 1-bit storage memory. The S=1 and R=1 case is a bit tricky: it produces an invalid state because we want the `Q` and `Q'` values to always be opposite, so in practice we avoid this case.

Mind that in the schema picture, we use `S'` and `R'` as inputs, they are the inverted values of `S` and `R` respectively.

]]

T.wiki_dlatch_name = "D Latch"
T.wiki_dlatch_text= [[

`D LATCH`

!img:wiki/imgs/mem2.png
We can extend the SR latch to create a `D`-Latch, where we have two inputs: a data input `D`, containing the bit to be assigned to the memory, and an `E` enable input, which will tell the memory to update or not (when E=0, the bit is not assigned and nothing happens). The output `Q` will represent the stored bit.
!img:wiki/imgs/mem3.png
It can be implemented as follows:
!img:wiki/imgs/mem4.png

Mind that Q can change at any time whenever `E`=1. This can be inconvenient sometimes. Imagine for example, that you pick the stored bit, do some calculation on it and want to store it again. If the E keeps active, the bit will be updated immediately, which will trigger the calculation again!

]]

T.wiki_dflipflop_name = "D Flip Flop"
T.wiki_dflipflop_text= [[

`D FLIP FLOP`

!img:wiki/imgs/mem8.png
Sometimes you want the memory to be updated only once when the clock (`CLK`) goes from 0 to 1.

Then, you can perform your calculations however you want, have the `D` bit modified with the new (next) value of the storage without interfering the current storage/calculation. Then, the storage is only updated again whenever CLK goes to 0 and then back again to 1! (ie, in the next `CYCLE`, creating a proper sequential mechanism)

!img:wiki/imgs/mem9.png

This can be achieved with a `D flip flop`, that can be created using two D latches.

!img:wiki/imgs/mem10.png

We call this behaviour a `rising edge-triggered` memory, in contrast with the previous `level-triggered` memory of `D Latches`. Below a comparison between the two storage modes:

!img:wiki/imgs/mem13.png

]]

T.wiki_synchronous_name = "Synchronous Circuits"
T.wiki_synchronous_text= [[

In this section, you'll see how to use memory to create `synchronous sequential` circuits.

`Sequential` as in, calculations and memory updates are always done in sequence: first you update the memory, then you perform calculations, then you update the memory again, and so on.
!img:wiki/imgs/sync4.png
`Synchronous` as in, every bit storage you have is updated once and at the `same time` for each `enable` cycle. We use a special bit to perform this synchronization, which we call `CLOCK` bit.  Whenever it goes up, the memories are updated (with the current input `D` value, which come from calculations from previous cycle) and it is only updated again when it goes to 0 and back to 1. This forms what we call a `clock cycle`. We can see that the actual "sequential" property of our system is tightly linked with the cycles of the clock: each cycle defines one "update->compute" step in our sequence .
!img:wiki/imgs/sync2.png
You can structure synchronous circuits in 2 major subcircuits: (i) the `combinatorial` subcircuit and (ii) the `memory` subcircuit:
!img:wiki/imgs/sync1.png
The role of the `memory` subcircuit is to store and update the memory of the system. It's very similar to a big D flip flop but instead of a single bit, it stores multiple bits `S`, and the output is just a stored version of the input `S`.

The role of the `combinatorial` subcircuit is to take the previous state `S_prv` as input, as well as some external input bits `Din` and generate both the bits corresponding to the next desired state `S_nxt` and the outputs of the system `Dout`. In this subcircuit there's no memory or "loops", just straight logic computation.

This way, once you're designing a sequential circuit, you can separate the logic from the storage and think in terms of: (i) what should the memory look like? (ii) how can I use the current memory (and inputs) to generate the next state of the memory (and outputs)?

`EXAMPLE`: Let's design a circuit with 1 bit input `A` and that outputs the bit `Y` that appeared 2 clock cycles ago. The circuit should look like:

1. CC=0 A=A0 --> Y=n/a
2. CC=1 A=A1 --> Y=n/a
3. CC=2 A=A2 --> Y=A0
4. CC=3 A=A3 --> Y=A1

We would need 2 bits for the memory: one `S0` for storing the current bit and one `S1` for the bit one clock ago. At the current clock cycle, you want to output the bit that appeared 1-clock "ago" from the last clock cycle, ie, the `S1` that you have as input. The updates then would look like the following:

- `S0_nxt = Din`
- `S1_nxt = S0_prv`
- `Dout = S1_prv`

And the circuit would look something like that:
!img:wiki/imgs/sync3.png

]]
T.wiki_meminit_name = "Memory Initialization"
T.wiki_meminit_text= [[

`MEMORY INITIALIZATION:`

Flip flops don't have an initial state: the latches start at an undefined state, so everytime you have a synchronous circuit, before doing any calculation you need to initialize your memory somehow.

That's why we often introduce an extra auxiliar bit called `POWER-ON-RESET` that is 1 for a few initial clock cycles, then become zero.

!img:wiki/imgs/sync6.png
!img:wiki/imgs/sync8.png

]]
T.wiki_propdelay_name = "Propagation Delay"
T.wiki_propdelay_text= [[

`PROPAGATION DELAY`

Both NAND gates and wires have propagation delay. When you chain components together, delays add up.

`When to AVOID clock delay:`
You should generally avoid adding gates between the clock signal and flip-flop inputs. If some flip-flops receive the clock later than others, your memory will no longer be synchronous and you might have bugs.

!img:wiki/imgs/sync5.png

`When to USE clock delay:`
However, there are cases where you WANT to delay the clock:

1. When you derive the clock from your data (like detecting button presses), the clock must arrive AFTER the data is stable. Use NOT-NOT chains to delay the clock.

2. To create gated clocks that reduce updates on unused circuit parts, saving simulation time (and power in real circuits).

In these cases, plan carefully and mind the propagation delays of your gates and wires.
]]

T.wiki_setuphold_name = "Setup and Hold Time"
T.wiki_setuphold_text = [[

`SETUP TIME AND HOLD TIME`

These are two critical timing requirements for flip-flops:

`Setup Time:`
The data input must be stable for a certain amount of time BEFORE the clock rising edge arrives. This is called the `setup time`. If the data changes too close to the clock edge, the flip-flop might capture an incorrect or unstable value.

`Hold Time:`
The data input must remain stable for a certain amount of time AFTER the clock rising edge. This is called the `hold time`. If the data changes too quickly after the clock edge, the flip-flop might not capture it correctly.

]]

T.wiki_halfadder_name = "Half Adder"
T.wiki_halfadder_text= [[

`HALF ADDER`

This circuit is called a HALF ADDER. It can be implemented using just two logic gates:
- An XOR gate produces the sum output
- An AND gate produces the carry output (which is 1 only when both inputs are 1)
!img:wiki/imgs/addition1.png
!img:wiki/imgs/addition2.png

]]

T.wiki_fulladder_name = "Full Adder"
T.wiki_fulladder_text = [[

`FULL ADDER`

A full adder extends the half adder by accepting a third input—the carry from a previous bit position. This allows you to chain multiple adders together to handle multi-bit numbers.

!img:wiki/imgs/addition3.png

]]

T.wiki_rcadder_name = "Ripple Carry Adder"
T.wiki_rcadder_text = [[

`MULTI-BIT ADDER`

You can create a multi-bit number adder combining multiple full-adders in series, combining the "carry output" from the previous digit to the "cary input" of the next digit.  This is also called a `Ripple-carry adder`.

!img:wiki/imgs/rc_adder.png

There are also other ways to create multi-bit adders, some more efficient. For instance, propagating the carries one by one can introduce a lot of delay when adding numbers with a lot of bits.
]]

T.wiki_bit_shifting_name = "Bit Shifting"
T.wiki_bit_shifting_text= [[

`BIT SHIFTING`

In this section we will explore how to shift bits in a number.

Shift operations basically "slide" the positions of the bits, either to the left or to the right. They are useful for many applications, including easy multiplication and division by a power of two: shifting a number one position to the left is roughtly equivalent to multiplying a number by two, shifting by two bits multiplying it by 4 and so on.

Shifting operations are typically represented by the "<<" or ">>" symbol: "A << n" represents shifting a number A n positions to the left, and "A >> n" n positions to the right.

For example, if `A`=00100 and `n`=1, shifting 1 bit to the right gives us: A >> n = 00010. Shifting 1 bit to left gives us: A << n = 01000.

Although shifting by a constant number can be done directly by wiring, shifting by "arbitrary" amounts can be more complex, and there are different ways to implement it. We describe two in more details in the sections `Barrel Shifter` and `Logarithmic shifter`.
]]

T.wiki_barrel_shifter_name = "Barrel Shifter"
T.wiki_barrel_shifter_text= [[

`BARREL SHIFTER`

A `barrel shifter` can perform any shift amount in a single operation using a crossbar network of multiplexers. For a 4-bit right shifter C = A >> B, the circuit works as follows:

!img:wiki/imgs/shift2.png

`Structure`:
- Create a grid where each output bit can be connected to any input bit
- Use multiplexers at each output position to select the correct input based on the shift amount B
- The shift amount B acts as the select signal for all multiplexers

!img:wiki/imgs/shift3.png

`How it works`:
- Each output bit position has a multiplexer that can choose from multiple input sources
- For a right shift by B positions, output[i] gets input[i+B] (with appropriate bounds checking)
- Positions that would receive bits from outside the input range are filled with zeros

`Key advantage`: Unlike simple shifters that shift one position per clock cycle, a barrel shifter can shift by any amount (0 to 3 positions for a 4-bit shifter) in one operation.

`Implementation`: The "crossbar" structure allows any input to route to any output through a network of multiplexers controlled by the shift amount.

!img:wiki/imgs/shift4.png

]]

T.wiki_logarithmic_shifter_name =  "Logarithmic Shifter"
T.wiki_logarithmic_shifter_text = [[

`LOGARITHMIC SHIFTER`

A `logarithmic shifter` is a clever way to shift bits by any amount using less components than a barrel shifter. The key idea is to break down the shift into smaller steps.

Think about it this way: instead of having one big circuit that can shift by 0, 1, 2, or 3 positions all at once, we use two smaller steps connected together. Each step makes a simple choice: "shift a bit, or don't shift".

For a 4-bit shifter C = A >> B (where B tells us how many positions to shift), the circuit works like this:

`Step-by-step process`:

Step 1: Look at B[0] (the first bit of B)
- If B[0] = 1: shift the bits right by 1 position
- If B[0] = 0: don't shift, keep bits as they are

Step 2: Take the result from Step 1, and look at B[1] (the second bit of B)
- If B[1] = 1: shift the bits right by 2 positions
- If B[1] = 0: don't shift, keep bits as they are

The final result is the output C!

!img:wiki/imgs/log_shift.png

`Example`: Let's say we want to shift right by 3 positions (B = 11 in binary):
- Step 1: B[0] = 1, so we shift by 1 position
- Step 2: B[1] = 1, so we shift by 2 more positions
- Total: We shifted by 1 + 2 = 3 positions!

`Another example`: To shift by 2 positions (B = 10 in binary):
- Step 1: B[0] = 0, so we don't shift
- Step 2: B[1] = 1, so we shift by 2 positions
- Total: We shifted by 0 + 2 = 2 positions!

`Why is this smart?`

Each step is very simple - it just decides between two options (shift or don't shift). By connecting these simple steps together, we can create any shift amount from 0 to 3.

For bigger numbers, you just add more steps: a step for "shift by 4", a step for "shift by 8", and so on. Each new step doubles the shift amount.

This is much simpler to build than a barrel shifter because each step only needs to make one simple choice, rather than choosing between many different options all at once!

]]

T.wiki_topic_basics = "Basics"
T.wiki_topic_gates = "Gates"
T.wiki_topic_mem = "Sequential Circuits"
T.wiki_topic_math = "Math"
