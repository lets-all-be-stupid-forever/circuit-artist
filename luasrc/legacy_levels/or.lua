local Tester = require 'tester'
local Clock = require 'clock'
local Or = Tester:extend()

function Or:new()
  Or.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 1, 'a'},
    {'output', 1, 'b'},
    {'input', 1, 'or_a_b'},
  }
  self.schedule = {
    {a=0, b=0, or_a_b=0},
    {a=1, b=0, or_a_b=1},
    {a=0, b=1, or_a_b=1},
    {a=1, b=1, or_a_b=1},
  }
end

addLevel({
  icon="../luasrc/imgs/levels/or_icon.png",
  name="OR Gate",
  desc=[[

!img:imgs/tutorial/or1.png

Objective: Compute the OR operation of the input pins `a` and `b` and write it to `or_a_b` output pin.

!hl

The OR gate returns a 1 if either input `a` OR input `b` is 1.

Example:
a=0 b=0 --> OR=0
a=0 b=1 --> OR=1
a=1 b=0 --> OR=1
a=1 b=1 --> OR=1
!img:imgs/tutorial/or2.png
An OR gate can be created using 3 NAND gates.

    ]],
    chips = {
      Clock(true),
      Or(),
    },
    id='OR',
})
