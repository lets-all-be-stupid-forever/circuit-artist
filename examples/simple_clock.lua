
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
  -- It stays up for 2 clock rising edges
  if cycle == 4 then
    WritePort(RST, 0)
  end
end

local function UpdateClock(cycle)
  -- rising edges happen on 2C+1 -> 2C+2
  WritePort(CLK, cycle % 2)
end

function _Update()
  UpdateReset(cycle)
  UpdateClock(cycle)
  if cycle == 20 then
    Pause();
  end
  cycle = cycle + 1
end

function _Draw()
end
