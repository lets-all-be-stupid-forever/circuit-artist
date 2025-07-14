
local Tester = require 'tester'
local Clock = require 'clock'
local rl = require 'raylib_api'
local ffi = require 'ffi'
local utils = require 'utils'
local Antenna = Tester:extend()

function Antenna :new()
  Antenna.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 2, 'signal_in'},
    {'output', 1, 'antenna_on'},
    {'input', 2, 'signal_out'},
  }

  -- Random sequence of numbers
  local seq={
    2, 1, 3, 0, 0, 0, 0, 1, 2, 1, 1 , 3 , 0, 1, 3 , 2, 1 ,0 , 3, 0, 1 , 2, 3, 2, 1, 0, 3, 2,
    1, 3, 0, 0, 0, 0, 1, 2, 1, 1 , 3 , 0, 1, 3 , 2, 1 ,0 , 3, 0, 1 , 2, 3, 2, 1, 0, 3, 2,
};

  -- 1 --> antenna on
  -- 0 --> antenna off
  local plan = {1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0,0};
  local iseq = 1;
  local prev = nil
  local schedule = {}
  for i=1,#plan do
    local act = plan[i];
    local sin = seq[iseq];
    local sout = nil
    iseq = iseq + 1;
    local name = nil;
    if act == 0 then
      name = 'Antenna off'
      sout = prev
    elseif act == 1 then
      name = 'Antenna on'
      sout = sin
      prev = sin
    end
    table.insert(schedule, {
      signal_in=sin,
      antenna_on=act,
      signal_out=sout,
      name=name,
    })
  end

  self.schedule = schedule;
end

addLevel({
  icon="../luasrc/imgs/levels/antenna_icon.png",
  name="Antenna Repeater",
  desc=[[

!img:imgs/levels/antenna_img1.png

Build a 2-bit signal antenna repeater.

The antenna is composed of 3 parts:
- A receiveing signal of 2 bits.
- An on-off state.
- An output signal of 2 bits.

The circuit should output the exact same signal when the antenna is on.

When the antenna is off, it should output the `LAST` received signal (i.e., it should memorize the last signal).

!hl

`Hint: D-Latch`

!img:imgs/tutorial/mem2.png
We can extend the SR latch to create a `D`-Latch, where we have two inputs: a data input `D`, containing the bit to be assigned to the memory, and an `E` enable input, which will tell the memory to update or not (when E=0, the bit is not assigned and nothing happens). The output `Q` will represent the stored bit.
!img:imgs/tutorial/mem3.png
It can be implemented as follows:
!img:imgs/tutorial/mem4.png

    ]],
    chips = {
      Clock(true),
      Antenna(),
    },
    id='ANTENNA',
})
-- Mind that Q can change at anytime whenever `E`=1. This can be inconvenient sometimes. Imagine for example, that you pick the stored bit, do some calculation on it and want to store it again. If the E keeps active, the bit will be updated immediately, which will trigger the calculation again!
