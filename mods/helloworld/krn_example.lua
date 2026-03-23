function _Setup()
  RST = AddPortOut(1, 'rst')
  CLK = AddPortOut(1, 'clk')
end

function _Start()
  cycle = 0
end

local function UpdateReset(cycle)
  if cycle == 0 then
    WritePort(RST, 1)
  end
  if cycle == 4 then
    WritePort(RST, 0)
  end
end

local function UpdateClock(cycle)
  WritePort(CLK, cycle % 2)
  if cycle == 40 then
    NotifyLevelComplete()
    Pause()
  end
end

function _Update()
  UpdateReset(cycle)
  UpdateClock(cycle)
  cycle = cycle + 1
end

function _Draw()
  -- Doesn't draw anything, but the function must be defined.
end

