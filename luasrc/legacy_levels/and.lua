local Tester = require 'tester'
local Clock = require 'clock'
local And = Tester:extend()

function And:new()
  And.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 1, 'a'},
    {'output', 1, 'b'},
    {'input', 1, 'and_a_b'},
  }
  self.schedule = {
    {a=0, b=0, and_a_b=0},
    {a=1, b=0, and_a_b=0},
    {a=0, b=1, and_a_b=0},
    {a=1, b=1, and_a_b=1},
  }
end

addLevel({
  icon="../luasrc/imgs/levels/and_icon.png",
  name="AND Gate",
  desc=[[

!img:imgs/tutorial/and1.png

Objective: Compute the AND operation of the input pins `a` and `b` and write it to `and_a_b` output pin.

!hl

The AND gate returns a 1 only if both inputs are 1, otherwise returns a 0. Ie, the output is only 1 if `a` AND `b` are 1.

Example:
a=0 b=0 --> AND=0
a=0 b=1 --> AND=0
a=1 b=0 --> AND=0
a=1 b=1 --> AND=1
!img:imgs/tutorial/and2.png
An AND gate can be created using 2 NAND gates, as shown below. That is equivalent to a NAND gate followed by a NOT gate.

    ]],
    chips = {
      Clock(true),
      And(),
    },
    id='AND',
})
