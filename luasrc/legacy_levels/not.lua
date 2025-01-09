local Tester = require 'tester'
local Clock = require 'clock'
local Not = Tester:extend()

function Not:new()
  Not.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 1, 'a'},
    {'input', 1, 'not_a'},
  }
  self.schedule = {
    {a=0, not_a=1},
    {a=1, not_a=0},
  }
end

addLevel({
  icon="../luasrc/imgs/levels/not_icon.png",
  name="NOT Gate",
  desc=[[

!img:imgs/tutorial/not1.png

Objective: Compute the NOT operation of the input pin `a` and write it to `not_a` output pin.

!hl

The not gate inverts the value of an input bit A.

Example:
a=0 --> NOT=1
a=1 --> NOT=0
!img:imgs/tutorial/not2.png
A not gate can be create using a single NAND gate, as shown in the picture above.

    ]],
    chips = {
      Clock(true),
      Not(),
    },
    id='NOT',
})
