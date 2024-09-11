local LevelComponent = require 'level_component'
local Clock = LevelComponent:extend()
local C = require 'c_api'
local S = C.GetSharedState()

function Clock:new(hidden)
  if hidden == nil then hidden = false end
  self.super.new(self)
  self.hidden = hidden
  if hidden then
    self.pins = {}
  else
    self.pins = {
      {'output', 1, 'power_on_reset'},
      {'output', 1, 'clock'},
    }
  end
end

function Clock:stopClock()
  self.stop = true
end

function Clock:onStart()
  S.elapsed = 0
  self.value = 0
  self.cycle = 0
  self.stop = false
end

function Clock:onTick(dt)
  S.elapsed = S.elapsed + dt
  while (S.elapsed > S.clock_time) and not self.stop do
    self.value = 1 - self.value
    S.elapsed = S.elapsed - S.clock_time
    if self.value == 1 then
      self.cycle = self.cycle + 1
    end
    self.dirty = true
  end
end

function Clock:getClockCount()
  return self.cycle
end


function Clock:getPowerOnReset()
  local por = 0
  if self.cycle < 2 then
    por = 1
  end
  return por
end

function Clock:onUpdate(prevIn, nextIn, output)
  local por = self:getPowerOnReset()
  if not self.hidden then
    self:setBits(output.power_on_reset, por)
    self:setBits(output.clock, self.value)
  end
end

-- Global function, useful to stop clock from anywhere
function stopClock()
  local clk = getLevelComponents()[1]
  clk:stopClock()
  local clock_count = clk:getClockCount()
  return clock_count
end


return Clock
