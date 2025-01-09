local Tester = require 'tester'
local Clock = require 'clock'
local Shifter = Tester:extend()
local math = require 'math'

local bit = require 'bit'
local band = bit.band
local lshift = bit.lshift

function Shifter:new()
  Shifter.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 4, 'a'},
    {'output', 2, 'n'},
    {'input', 4, 'a_lshift_n'},
  }
  self.schedule = { }
  math.randomseed(1)
  for a=0,15 do
  for n=0,3 do
    local r = band(lshift(a, n), 15)
    local c = {a=a, n=n, a_lshift_n=r, name=a .. ' << ' .. n .. ' = ' .. r}
    table.insert(self.schedule, c)
  end
  end
end

addLevel({
  icon="../luasrc/imgs/levels/shifter_icon.png",
  name="Bit Shifter",
  desc=[[

Objective: Shift the bits of a 4-bit input (`a`) to the left by a given ammount (`n`). The operation is represented by `a << n`.

Example:
- 0 << 0 = 0 (no shift)
- 1 << 0 = 1 (no shift)
- 2 << 1 = 4, because 2 = `0010`, and shifting 1 place to the left we get `010`0.
- 8 << 1 = 0, because 8 = `1000`, and shifting 1 place to the left we get `000`0.
- 3 << 2 = 12, because 3 = `0011`, and shifting 2 places to the left we get `11`00.

!hl

In this section we will see how to `shift` bits in a number.

The `shift` operation is simply moving the bits either to the left or to the right, as in the example below:

!img:imgs/tutorial/shift1.png

The shift operation is often represented by `A << b` when `A` is shifted by `b` bits to the left and `A >> b` when it is shifted to the right.

You can solve it by using a grid-like structure called `barrel shifter`. In order to understand how it works, let's look at a 4-bits right shifter `C = A >> B`. We would want the circuit for each shift value `B`  to look more or less like the following:

!img:imgs/tutorial/shift3.png

This can be achieved by creating a cell, then reproduce it in a grid-like structure like the picture below, then you create activation bit flags for each possible shift and link them to the associate cells. The function of each cell is to create the "L"-like curves as in the circuit above.

!img:imgs/tutorial/shift2.png

We can build each cell the following way:

!img:imgs/tutorial/shift4.png


    ]],
    chips = {
      Clock(true),
      Shifter()
    },
    id='SHIFTER',
})



