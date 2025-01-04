local Tester = require 'tester'
local Clock = require 'clock'
local Wires = Tester:extend()

function Wires:new()
  Wires.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 1, 'a_in'},
    {'output', 2, 'b_in'},
    {'output', 4, 'c_in'},
    {'input', 1, 'a_out'},
    {'input', 4, 'c1_out'},
    {'input', 2, 'b_out'},
    {'input', 4, 'c2_out'},
  }

  self.schedule = {}
  for a=0,1 do
    for b=0,3 do
      for c=0,15 do
        table.insert(
          self.schedule, {
            a_in=a,
            b_in=b,
            c_in=c,
            a_out=a,
            b_out=b,
            c1_out=c,
            c2_out=c,
          }
        )
      end
    end
  end
end


addLevel({
  icon="../luasrc/imgs/levels/wires_icon.png",
  name="Wires",
  desc=[[

!img:imgs/levels/wires_icon.png

Objective: Connect input pins to output pins using pixel wires.

- `a_in` should be connected to `a_out`.
- `b_in` should be connected to `b_out`.
- `c_in` should be connected to `c1_out` and `c2_out`.

!img:imgs/levels/wires_pins_example.png

You can use the `line tool` to help with the multi-bit wires.

For multi-bit pins, the lowest bit is the upper one both in the input and ouput, so they should be connected in the same order, as in the example below:

!img:imgs/levels/wires_example.png

!hl

Wires are formed by connected non-black pixels. Wires can have 4 states, as displayed below:
- `ON` (lighter color)
- `OFF` (darker color)
- `UNDEFINED` (magenta color)
- `ERROR` (red color)
!img:imgs/tutorial/wires1.png
The `ON` state represents the logic 1, and the `OFF` state represents the logic 0. Wires state can be toggled (1->0 or 0->1) by clicking on it during simulation mode.
!img:imgs/tutorial/wires2.png
The `ERROR` wires only appear when an error occurs during simulation. There are 2 possible errors that can occur:
Error type 1: A wire that belongs to the output of more than one gate. Example below. Each wire can only be the output of maximum 1 gate.
!img:imgs/tutorial/simu_error1.png
Error type 2: When the wire state oscillates between different values without end. Example below.
!img:imgs/tutorial/simu_error_cycle.png
The `UNDEFINED` state means that we don't know the state of the wire. Every wire start with that state, with exception of wires that are not output of a NAND gate. Those wires are initialized as 0.
Below an example of how the wire states change when the simulation is first launched:
!img:imgs/tutorial/simu_example1.png
The width or shape of a wire is not important, nor its color, as long as the pixels are connected.
!img:imgs/tutorial/wires3.png
However, wires are allowed to cross. As a rule of thumb, straight wires only connect at T or L connections.
!img:imgs/tutorial/wire_cross.png
Wires are only connected via up/down/left/right pixel neighbours. Diagonal pixels are not considered connected. In the example below, in the first case you have 4 different wires, while in the second example you have a single wire.
!img:imgs/tutorial/wire_diagonal.png

    ]],
    chips = {
      Clock(true),
      Wires(),
    },
    id='WIRES',
})

