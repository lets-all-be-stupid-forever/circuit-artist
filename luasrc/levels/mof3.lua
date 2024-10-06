local Tester = require 'tester'
local Clock = require 'clock'

local MultipleOf3 = Tester:extend()

function MultipleOf3:new()
  MultipleOf3.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 8, 'n'},
    {'input', 1, 'mof3'},
  }
  local cases = { 1, 2, 4, 8, 16, 32, 64, 128, 0, 255, 123, 45, 111, 100, 48, 3, 33, 31, 129}
  local schedule = {}
  for i=1,#cases do
    local n = cases[i]
    local ans = 0
    if (n % 3) == 0 then
      ans = 1
    end

    local extra = '(multiple of 3)'
    if ans ~= 1 then
      extra = '(not multiple of 3)'
    end
    table.insert(schedule, {n=n, mof3=ans, name='n=' .. n .. ' ' .. extra})
  end
  self.schedule = schedule
end

addLevel({
    icon = "../luasrc/imgs/levels/mof3_icon.png",
    name = "Multiple of 3",
    desc = [[

!img:imgs/levels/mof3_img.png

Check if a number is a multiple of 3.

You're given as input an 8-bit unsigned positive number `n` and should return a 1-bit result `mof3` with 1 if `n` is a multiple of 3, and 0 otherwise.

Examples:

n=0 --> mof3=1
n=1 --> mof3=0
n=2 --> mof3=0
n=44 --> mof3=0
n=111 --> mof3=1

]],
    chips = {
      Clock(true),
      MultipleOf3(),
    },
    id='MOF3',
    unlockedBy='BUS',
})
