local Tester = require 'tester'
local Clock = require 'clock'
local Mux = Tester:extend()

function Mux:new()
  Mux.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 1, 'a'},
    {'output', 1, 'b'},
    {'output', 1, 's'},
    {'input', 1, 'mux'},
  }
  self.schedule = {
    {a=0, b=0, s=0, mux=0},
    {a=1, b=0, s=0, mux=1},
    {a=0, b=1, s=0, mux=0},
    {a=1, b=1, s=0, mux=1},
    {a=0, b=0, s=1, mux=0},
    {a=1, b=0, s=1, mux=0},
    {a=0, b=1, s=1, mux=1},
    {a=1, b=1, s=1, mux=1},
  }
end

addLevel({
  icon="../luasrc/imgs/levels/mux_icon.png",
  name="MUX",
  desc=[[

!img:imgs/tutorial/mux1.png

Objective: Implement a 2:1 multiplexer: given two input bits `a` and `b`, if `s` is off, return the value of `a`, otherwise return the value of `b`.

!hl

A `MUX` is a like a bit `selector`: Given 2 input bits `a` and `b`, and a selection bit `s`, if `s` is 0, `MUX` returns the value of `a`, and if `s` is 1, `MUX` returns the value of `b`.
!img:imgs/tutorial/mux3.png
Muxes work like selectors (or a "switch"). They're useful for example when you have multiple calculations and want to select only one of the results. It's like an "if" statement in programming: IF S=0, RETURN A; ELSE IF S=1 RETURN B;.
!img:imgs/tutorial/mux2.png
A mux can be implemented as shown in the diagram above.

    ]],
    chips = {
      Clock(true),
      Mux(),
    },
    id='MUX',
})


