local Tester = require 'tester'
local Clock = require 'clock'
local math = require 'math'
local utils = require 'utils'

local SimpleRam = Tester:extend()

local bit = require 'bit'
local bnot = bit.bnot
local band, bor, bxor = bit.band, bit.bor, bit.bxor
local lshift, rshift, rol = bit.lshift, bit.rshift, bit.rol


function SimpleRam:new()
  self.super.new(self)
  self.pins = {
    {'output', 1, 'we'},
    {'output', 6, 'wa'},
    {'output', 8, 'wd'},
    {'input', 8, 'rd'},
  }

  local cases = {
    {we=0, wa=0, wd=0x00000000, name='Initializing'},
  }
  local db = {}
  math.randomseed(0)

  -- first set all registers with a random number
  local N = 64
  for i = 1, N do
    local r = i
    db[i] = r
    table.insert(cases, {we=1, wa=i-1, wd=r, msg=msg, name='MEM['..  i- 1 .. ']=' .. r})
  end

  -- Now queries each register
  for j = 1, 4*N do
    local wd = utils.randomBits(8)
    local wa = math.random(1, N)
    local we = math.random(1, 2) - 1
    -- we = 0
    if we == 1 then
      local msg = 'MEM[' .. wa-1 .. ']=' .. wd
      table.insert(cases, {we=1, wa=wa-1, wd=wd, name=msg})
      db[wa] = wd
    else
      local msg = 'READ MEM[' .. wa-1 .. '] (should be ' .. db[wa] .. ')'
      table.insert(cases, {we=0, wa=wa-1, wd=0, rd=db[wa], name=msg})
    end
  end
  self.schedule = cases
end

return {
    icon = "../luasrc/imgs/levels/simple_ram_icon.png",
    name = "Memory: 8bit SRAM",
    desc=[[

!img:imgs/levels/mem64.png


Write a memory for bytes (8-bit) with 64 addresses. (ie, total storage of 64x8 = 512 bits).

The input bit flag `we` defines if the operation should be a write (`we`=1) or a read (`we`=0).

In `read` operations (`we`=0) , we just return the value of the byte at address `wa` in `rd`.

In `write` operations (`we`=1) we assign the value `wd` to the address `wa`.

In other words:

- `we`=0 (read) --> `rd`=MEM[`wa`]
- `we`=1 (write) --> MEM[`wa`]=`wd`

Inputs:
- `we` (1bit): write enabled
- `wa` (8bits): data address
- `wd` (8bits): write data

Output:
- `rd` (8bits): read data

The initial state of the memory is not important.

`!! Attention !!`: The memory change should take place during the rising edge of the clock, ie, exactly when the clock value is passing from 0 to 1.

For the tests, we will fill each memory address with random values, and then on a second time we will try to access each address to see if the values match.

 Then, we will randomly assign and query again for a few times before end.

    ]],
    chips = {
      Clock(),
      SimpleRam(),
    }
}
