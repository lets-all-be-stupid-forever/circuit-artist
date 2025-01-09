local Tester = require 'tester'
local Clock = require 'clock'
local Comparator = Tester:extend()
local math = require 'math'

function Comparator:new()
  Comparator.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 4, 'a'},
    {'output', 4, 'b'},
    {'input', 1, 'a_gt_b'},
    {'input', 1, 'a_eq_b'},
    {'input', 1, 'a_lt_b'},
  }
  self.schedule = {
    {a=0, b=0, a_gt_b=0, a_eq_b=1, a_lt_b=0, name='a=0 b=0'},
    {a=1, b=0, a_gt_b=1, a_eq_b=0, a_lt_b=0, name='a=1 b=0'},
    {a=0, b=1, a_gt_b=0, a_eq_b=0, a_lt_b=1, name='a=0 b=1'},
  }
  math.randomseed(1)
  for i=1,64 do
    local a = math.random(16) - 1
    local b = math.random(16) - 1
    local c = {a=a, b=b, a_gt_b=0, a_eq_b=0, a_lt_b=0, name='a='..a..' b='..b}
    if a > b then c['a_gt_b'] = 1 end
    if a == b then c['a_eq_b'] = 1 end
    if a < b then c['a_lt_b'] = 1 end
    table.insert(self.schedule, c)
  end
end

addLevel({
  icon="../luasrc/imgs/levels/comparator_icon.png",
  name="A > B",
  desc=[[

Objective: Compare two unsigned 4-bit numbers `a` and `b` and tell if `a` is bigger (`a_gt_b`), equals (`a_eq_b`) or smaller (`a_lt_b`) then `b`.

For example:

- `a`=0 `b`=0 --> `a_gt_b`=0 `a_eq_b`=1 `a_lt_b`=0
- `a`=1 `b`=0 --> `a_gt_b`=1 `a_eq_b`=0 `a_lt_b`=0
- `a`=0 `b`=1 --> `a_gt_b`=0 `a_eq_b`=0 `a_lt_b`=1

!hl

This section describes how to compare two positive integer numbers `A` and `B`. We often want to do the 3 comparisons at a time, (i) `A > B`, (ii) `A = B` and (iii) `A < B`.

We start creating a comparator for a single bit:
  !img:imgs/tutorial/comparator1.png
Which can be done as follows:
  !img:imgs/tutorial/comparator2.png
  !img:imgs/tutorial/comparator3.png
Then, in a second step, we extend this 1-bit comparator to accept inputs from a "more significant" bit. The idea is to do it the same way we do to compare two numbers: first we check if the most significant digit is equal, lower or higher than the other: if it's lower or higher we know whether a number is lower or higher, but if they're equal, we need to move forward to the next digit, creating a sequence of comparisons.
  !img:imgs/tutorial/comparator4.png
The formulas will look like the following, where `Aprv` = previous A, ie, result from previous comparator (from a more signficant bit), and `Anxt` = next A, ie, result going to next comparator (towards a least significant bit).

`A`<`Bnxt` = (`A`<`Bprv`) OR (`A`=`Bprv` AND `A`<`B`)
`A`=`Bnxt` = (`A`=`Bprv`) AND (`A`=`B`)
`A`>`Bnxt` = (`A`>`Bprv`) OR (`A`=`Bprv` AND `A`>`B`)

Then, similarly to the addition, we chain these comparators to create a bigger comparator for N bits.
  !img:imgs/tutorial/comparator5.png

    ]],
    chips = {
      Clock(true),
      Comparator(),
    },
    id='COMPARATOR',
})


