local Tester = require 'tester'
local Clock = require 'clock'
local math = require 'math'
local GCD = Tester:extend()

local function gcd(a,b)
  if type(a) == "number" and type(b) == "number" and
    a == math.floor(a) and b == math.floor(b) then
    if b == 0 then
      return a
    else
      return gcd(b, a % b) -- tail recursion
    end
  else
    error("Invalid argument to gcd (" .. tostring(a) .. "," ..
    tostring(b) .. ")", 2)
  end
end

function GCD:new()
  GCD.super.new(self)
  self.has_submit = true
  self.pins = {
    {'input', 1, 'submit'},
    {'output', 16, 'a'},
    {'output', 16, 'b'},
    {'input', 16, 'gcd_ab'},
  }
  math.randomseed(0)
  local cases = {}
  local k = math.floor(32000 / (24*24))
  for i=1,24 do
    local a = math.random(k * i * i) + 1
    local b = math.random(k * i * i) + 1
    local gcd_ab = gcd(a,b)
    cases[i] = {
      a=a,
      b=b,
      gcd_ab=gcd_ab,
      name='GCD(a=' .. a .. ', b=' .. b .. ') should be ' .. gcd_ab,
    }
  end
  self.schedule = cases
end

return {
    icon = "../luasrc/imgs/levels/gcd_icon.png",
    name = "Greatest Common Divisor",
    desc=[[

!img:imgs/levels/gcd_img.png

Find the greatest common divisor (GCD) between two positive intergers.

The GCD between two numbers is the biggest positive number that divides both numbers. Examples:

`GCD(10, 6)=2` --> because 2 divides both 10 and 6 since 10 = 2x5 and 6 = 2x3.
`GCD(20, 8)=4`  --> 20 = 4x5 and 8=4x2.
`GCD(15,14)=1` --> There's no positive number that divides 15 and 14 besides 1.
`GCD(8,8)=8` --> A number is divisible by itself.
`GCD(24, 8)=8`

You'll be given as input two 8-bit positive numbers `A` and `B`, and you should compute in output `gcd_ab` the 8-bit number corresponding to its GCD.

You can perform the calculation in multiple clock cycles, and the result is only verified when the `submit` flag (1bit output) will be 1. The result is verified in the rising edge of the clock.

`Tips`: You can use the Euclidean Algorithm to compute the GCD.

Basically, if `A` and `B` are positive numbers, then if you subtract both numbers, the GCD between them will remain the same.

Using this property, you can create a sequence starting with two numbers A and B, then replace the biggest number by the difference between it and the smaller one. You can see that this sequence will eventually stop, because the numbers will always be smaller at each step. Repeating this operation, you'll get at the end equal numbers, which will correspond to the GCD you're seeking (one step before reaching 0).

IF A>B THEN A = A - B
IF B>A THEN B = B - A
IF A=B THEN GCD=A

For example, computing the GCD between 10 and 6:

`STEP1`: A=10, B=6 --> A is bigger, so we make A=A-B=10-6=4

`STEP2`: A=4, B=6 --> B is bigger, so we make B=B-A=6-4=2

`STEP3`: A=4, B=2 --> A is bigger, so we make A=A-B=4-2=2

`STEP4`: A=2, B=2 --> A and B are equal, so the GCD must be 2!


]],
  chips = {
    Clock(),
    GCD(),
  }
}
