--[[
A level similar to original Sandbox but:
- Clock updates at a fixed rate of 30 ticks instead of waiting the circuit stops.
- It pauses the game after 10 clock cycles.
- It runs the simulation at a sligther slower pace (base Ticks Per Second is
  set to 100, while default is 240)
]]

function _Setup()
  -- Port Out = Output of the script = entering the image.
  -- The call to AddPortOut returns the ID of the port, used later to write or
  -- read, depending on the type of port.
  RST = AddPortOut(1, 'rst')
  CLK = AddPortOut(1, 'clk')
end

function _Start()
  cycle = 0
  -- If SetUpdateInterval is set to 0, it will only call update when circuit
  -- stops updating. If a number >0 is passed here, it is called every N ticks.
  SetUpdateInterval(16)
  -- Sets ticks per second slight slower just to show it's possible to change
  -- speed.
  -- In practice one might want to try higher TPS for very fast circuits.
  SetBaseTPS(100)
end

local function UpdateReset(cycle)
  if cycle == 0 then
    -- Writes to the RST port
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
    -- Pauses the simulation
    Pause();
  end
  cycle = cycle + 1
end

function _Draw()
  -- Doesn't draw anything, but the function must be defined.
end
