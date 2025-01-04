local Tester = require 'tester'
local Clock = require 'clock'
local Xor = Tester:extend()

function Xor:new()
  Xor.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 1, 'a'},
    {'output', 1, 'b'},
    {'input', 1, 'xor_a_b'},
  }
  self.schedule = {
    {a=0, b=0, or_a_b=0},
    {a=1, b=0, or_a_b=1},
    {a=0, b=1, or_a_b=1},
    {a=1, b=1, or_a_b=0},
  }
end

addLevel({
  icon="../luasrc/imgs/levels/xor_icon.png",
  name="XOR Gate",
  desc=[[

!img:imgs/tutorial/xor1.png

Objective: Compute the XOR operation of the input pins `a` and `b` and write it to `xor_a_b` output pin.

!hl

The XOR gate returns 1 if exactly one of them is 1, otherwise returns a 0. (it's like a bit sum without carry)

Example:
a=0 b=0 --> XOR=0
a=0 b=1 --> XOR=1
a=1 b=0 --> XOR=1
a=1 b=1 --> XOR=0
!img:imgs/tutorial/xor2.png
A XOR gate can be created using 4 NAND gates.

    ]],
    chips = {
      Clock(true),
      Xor(),
    },
    id='XOR',
})
