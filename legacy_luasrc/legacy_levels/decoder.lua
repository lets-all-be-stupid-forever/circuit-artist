local Tester = require 'tester'
local Clock = require 'clock'
local Decoder = Tester:extend()

function Decoder:new()
  Decoder.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 2, 'a'},
    {'input', 4, 'decoder'},
  }
  self.schedule = {
    {a=0,  decoder=1},
    {a=1,  decoder=2},
    {a=2,  decoder=4},
    {a=3,  decoder=8},
  }
end

addLevel({
  icon="../luasrc/imgs/levels/decoder_icon.png",
  name="Decoder",
  desc=[[

!img:imgs/levels/decoder_icon.png

Objective: Implement a 2:4 decoder: given a 2-bit input `a`, activate only one bit in the output out of the 4, corresponding to the value of the number in `a`:

- a=00 --> decoder[0]=1, or, decoder=0001
- a=01 --> decoder[1]=1, or, decoder=0010
- a=10 --> decoder[2]=1, or, decoder=0100
- a=11 --> decoder[3]=1, or, decoder=1000

!hl

!img:imgs/tutorial/decoder1.png

A decoder transform an input binary number into a new binary number containing exactly 1 bit active, identifying the input.
Example:
`A`=0 (0) --> `D`=01 (1)
`A`=1 (1) --> `D`=10 (2)
!img:imgs/tutorial/decoder2.png
A 1-bit input decoder can be implemented as in the example above.

    ]],
    chips = {
      Clock(true),
      Decoder(),
    },
    id='Decoder',
})
