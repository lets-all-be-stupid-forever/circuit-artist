local LevelComponent = require 'level_component'
local Clock = require 'clock'

local BasicROM = LevelComponent:extend()

function BasicROM:new()
  BasicROM.super.new(self)
  self.pins = {
    {'input', 8, 'rom_ra'}, -- Read address
    {'output', 8, 'rom_rd'}, -- memory value
  }
end

function BasicROM:onStart()
  -- One could extend this code to read the ROM from a text file whenever
  -- simulation starts from example.
  local mem = {}
  for i=0,255 do
    mem[i] = i
  end
  self.memory = mem
end


-- Just returns the values defined on the current test case.
function BasicROM:onUpdate(prevIn, nextIn, output)
  local addr = self:toNumber(nextIn.rom_ra)
  local memValue = self.memory[addr]
   self:setBits(output.rom_rd, memValue)
end

return BasicROM
