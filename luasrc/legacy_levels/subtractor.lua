local Tester = require 'tester'
local Clock = require 'clock'
local Subtractor = Tester:extend()
local math = require 'math'

local function fmt(n)
  if n >= 0 then return tostring(n) end
  return '(' .. n .. ')'
end

function Subtractor:new()
  Subtractor.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 8, 'a'},
    {'output', 8, 'b'},
    {'input', 8, 'a_minus_b'},
  }
  self.schedule = {
    {a=0, b=0, a_minus_b=0, name='0 - 0'},
    {a=1, b=0, a_minus_b=1, name='1 - 0'},
    {a=0, b=1, a_minus_b=-1, name='0 - 1'},
    {a=1, b=1, a_minus_b=0, name='1 - 1'},
    {a=-1, b=-1, a_minus_b=0, name='(-1) - (-1)'},
    {a=127, b=-128, a_minus_b=255, name='127 - (-128)'},
    {a=-128, b=127, a_minus_b=1, name='(-128) - 127'},
  }
  math.randomseed(1)
  for i=1,64 do
    local a = math.random(256) - 128
    local b = math.random(256) - 128
    local s = a-b
    if s < -128 then s = 256 + s end
    if s > 127 then s = s - 256 end
    name = fmt(a) .. ' - ' .. fmt(b)
    table.insert(self.schedule, {a=a, b=b, a_minus_b=s, name=name})
  end
end

addLevel({
  icon="../luasrc/imgs/levels/subtractor_icon.png",
  name="A - B",
  desc=[[

Objective: Subtract two `signed` 8-bit numbers.

We want `a_minus_b` = `a` - `b`.

!hl

In order to represent `signed integers`, we create a sort of extension of the unsigned integer definition:
!img:imgs/tutorial/numbers2.png
This representation is also called `two's complement`.

The formula to convert a number to its negative is given as: `-N = NOT(N) + 1`. It works for both positive and negative numbers.

So let's say you want to find -3 in a 4-bit representation. You know that 3 is represented in positive integers by 0011. So, in order to find -3 first you find NOT(3) by inverting each bit, getting 1100. Then, you add 1 to it, getting 1100 + 1 = 1101, so the representation of -3 is 1101. You can also do the other way around, to get -(-3): First you invert 1101 to get 0010, then you add 1, to get 0010 + 1 = 0011, which is our starting representation of 3.

!hl

The subtraction of two integer numbers is very similar to its addition. All you need to do is invert the sign of the second number and sum them, as in `A-B = A + (-B)`, but since -N = NOT(N) + 1, all you need to do is A-B = A + NOT(B) + 1.

This can be done by using the "carry in" of the first `FULL ADDER` in the n-bits adder.

  !img:imgs/tutorial/sub2.png

    ]],
    chips = {
      Clock(true),
      Subtractor(),
    },
    id='SUBTRACTOR',
})
