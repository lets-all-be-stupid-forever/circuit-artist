local Tester = require 'tester'
local Clock = require 'clock'
local Nand = Tester:extend()

function Nand:new()
  Nand.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 1, 'a'},
    {'output', 1, 'b'},
    {'input', 1, 'nand_a_b'},
  }
  self.schedule = {
    {a=0, b=0, nand_a_b=1},
    {a=1, b=0, nand_a_b=1},
    {a=0, b=1, nand_a_b=1},
    {a=1, b=1, nand_a_b=0},
  }
end

addLevel({
  icon="../luasrc/imgs/levels/nand_icon.png",
  name="NAND Gate",
  desc=[[

!img:imgs/tutorial/nand1.png

Objective: Compute the NAND operation of the input pins `a` and `b` and write it to `nand_a_b` output pin.

!hl

A NAND logic gate is represented by 3 pixels, as shown below.

!img:imgs/tutorial/nand0.png

A logic gate is a "mechanism" that transforms one or two bit inputs into one bit output.

The NAND logic gate has 2 inputs and 1 output. It "reads" the value of the inputs and, depending on their value, it will assign a value to the output wire. The assignment table can be found below:

a=0 b=0 --> NAND=1
a=0 b=1 --> NAND=1
a=1 b=0 --> NAND=1
a=1 b=1 --> NAND=0

For example, if the 2 inputs are 0, then the output will be assigned to 1. See examples below.

!img:imgs/tutorial/nand2.png

The NAND gates can be drawn in any orientation (facing up, down, left or right):

!img:imgs/tutorial/nand3.png

Every non-black pixel in the image that is not part of a NAND gate is considered a wire, and has some state associated to it, as described below.


    ]],
    chips = {
      Clock(true),
      Nand(),
    },
    id='NAND',
})
