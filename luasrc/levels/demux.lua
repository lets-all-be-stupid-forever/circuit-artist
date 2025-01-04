local Tester = require 'tester'
local Clock = require 'clock'
local Demux = Tester:extend()

local function pow2(n)
  if n == 0 then return 1;
  elseif n == 1 then return 2;
  elseif n == 2 then return 4;
  elseif n == 3 then return 8;
  end
end

function Demux:new(bits_in, bits_sel)
  Demux.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', bits_in, 'a'},
    {'output', bits_sel, 's'},
  }

  local num_out = pow2(bits_sel)
  local num_a= pow2(bits_in)

  for i=1,num_out do
    table.insert(self.pins, {'input', bits_in, 'd' .. i-1})
  end

  self.schedule = {}
  for a=0,num_a-1 do
    for s=1,num_out do
      local case = {a=a, s=s-1}
      for ss=1, num_out do
        case['d' .. ss-1] = 0
      end
      case['d' .. s-1] = a
      table.insert(self.schedule, case)
  end
  end
end

addLevel({
  icon="../luasrc/imgs/levels/demux_icon.png",
  name="DEMUX",
  desc=[[

!img:imgs/levels/demux_icon.png

Objective: Implement a demux: given a multi-bit input `a`, return it to the n-th output `d_n` if the value of `s` is `n`.

For example:

- `s=0` --> `d0`=`a`, `d1`=0
- `s=1` --> `d0`=0, `d1`=`a`

!hl

!img:imgs/tutorial/demux1.png

A `DEMUX` is like a bit `router`: Given an input bit `a`, 2 outputs `Y0` and `Y1` and a selector bit `s`, if `s` is 0, then set `Y0` to the value of `a` and `Y1` to 0. Otherwise, if `s` is 1, set `Y0` to 0 and `Y1` to the value of `a`.
!img:imgs/tutorial/demux3.png
Demuxes work like routers: you have a wire and the selector bit chooses to which wire the input bit should go.
Example:
a=0, s=0 --> Y0 = 0, Y1 = 0
a=1, s=0 --> Y0 = 1, Y1 = 0
a=0, s=1 --> Y0 = 0, Y1 = 0
a=1, s=1 --> Y0 = 0, Y1 = 1
A demux can be implemented as follows:
!img:imgs/tutorial/demux2.png

    ]],
    chips = {
      Clock(true),
      Demux(1, 1),
    },
    id='DEMUX',
})

