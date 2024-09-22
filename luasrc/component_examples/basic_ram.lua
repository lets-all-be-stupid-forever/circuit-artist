local LevelComponent = require 'level_component'
local Clock = require 'clock'

local BasicRAM = LevelComponent:extend()

function BasicRAM:new()
  BasicRAM.super.new(self)
  self.pins = {
    {'input', 8, 'ram_wa'},
    {'input', 8, 'ram_wd'},
    {'input', 1, 'ram_we'},
    {'output', 8, 'ram_rd'},
  }
  local mem = {}
  for i=1,256 do
    mem[i] = 0
  end
  self.memory = mem
end

function BasicRAM:onStart()
  -- initializes with 0
  for i=1,256 do
    self.memory[i] = 0
  end
  self.rd = 0
end

function BasicRAM:onClock(inputs, reset)
  if reset then
    return
  end
  local addr = self:toNumber(inputs.ram_wa)
  local we = self:toNumber(inputs.ram_we)
  self.addr = addr + 1
  if we == 1 then
    local wd = self:toNumber(inputs.ram_wd)
    self.memory[addr + 1] = wd
  end
  self.rd = self.memory[addr + 1]
  self.dirty = true
end

-- Just returns the values defined on the current test case.
function BasicRAM:onUpdate(prevIn, nextIn, output)
   self:setBits(output.ram_rd, self.rd)
end

return BasicRAM
