local Tester = require 'tester'
local Clock = require 'clock'
local Adder = Tester:extend()
local math = require 'math'

function Adder:new()
  Adder.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 4, 'a'},
    {'output', 4, 'b'},
    {'input', 4, 'a_plus_b'},
    {'input', 1, 'carry'},
  }
  self.schedule = {
    {a=0, b=0, a_plus_b=0, carry=0, name='0 + 0'},
    {a=1, b=0, a_plus_b=1, carry=0, name='1 + 0'},
    {a=0, b=1, a_plus_b=1, carry=0, name='0 + 1'},
    {a=1, b=1, a_plus_b=2, carry=0, name='1 + 1'},
    {a=15, b=0, a_plus_b=15, carry=0, name='15 + 0'},
    {a=15, b=1, a_plus_b=0, carry=1, name='15 + 1'},
    {a=15, b=15, a_plus_b=14, carry=1, name='15 + 15'},
  }
  math.randomseed(1)
  for i=1,64 do
    local a = math.random(16) - 1
    local b = math.random(16) - 1
    local s = a+b
    local c = 0
    if s >= 16 then
      c = 1
      s = s - 16
    end
    name = a .. ' + ' .. b
    table.insert(self.schedule, {a=a, b=b, a_plus_b=s, carry=c, name=name})
  end
end

addLevel({
  icon="../luasrc/imgs/levels/adder_icon.png",
  name="A + B",
  desc=[[

Objective: Add two unsigned 4-bit numbers.

We want `a_plus_b` = `a` + `b`, and if the sum uses more than 4 bits, we should return the first 4 bits of the results and activate a carry flag `carry=1`.


!hl

We can represent `unsigned integer` numbers by simply counting them in increasing order from 0 on. By doing that, every bit will represent a power of two.
!img:imgs/tutorial/numbers1.png
So for example:
101 = 1*100 + 0*10 + 1*1 = 1*4 + 0*2 + 1*1 = 5 (3 bits number)
0100 = 0*1000 + 1*100 + 0*10 + 0*1 = 0*8 + 1*4 + 0*2 + 0*1 = 4 (4 bits number)

!hl

In this section we will see how to `add` two integers of `N` bits.

First, we have to create a sub-circuit that perform the addition of two bits, and outputs both the result, and an extra "carry", as you would do when summing numbers by hand. We call this sub-circuit a `HALF ADDER`.
!img:imgs/tutorial/addition1.png
That can be created as follows: it's just a XOR for the addition, with an extra AND to add a carry whenever both inputs are 1 (only situation where we have a carry=1).
!img:imgs/tutorial/addition2.png
Having the carry as output is not enough. Now we need to extend the subcircuit to accept a carry input as well, as we do when we are summing numbers by hand. This extended sub-circuit is called a `FULL ADDER`, and can be created as follows. Basically instead of summing `a` and `b`, we will sum `a`, `b` and `carry_in` bits.
!img:imgs/tutorial/addition3.png
Once we have the full adder, we can combine them together to create an adder for N-bits, as follows:
!img:imgs/tutorial/addition4.png

    ]],
    chips = {
      Clock(true),
      Adder(),
    },
    id='ADDER',
})

