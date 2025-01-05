-- Level Component Class
-- ====================
--
-- This is the root class of level components.
-- Level components are a set of wires that update by themselves.
-- You can stack multiple components in a single level, and each will be
-- updated independently (although they can reach each other via
-- getLevelComponents())
--
-- It can be useful if you want to re-use the components across levels (like
-- the clock component), and it can be useful if a component updates way more
-- often than others, you don't need to run the update code of everything in
-- your level whenever any wire changes, which can be more efficient.
--
-- A Level is just a lits of level components.
--
local Object = require "classic"
local ffi = require 'ffi'
local utils = require 'utils'
local C = require 'c_api'
local bit = require 'bit'
local bnot = bit.bnot
local band, bor, bxor = bit.band, bit.bor, bit.bxor
local lshift, rshift, rol = bit.lshift, bit.rshift, bit.rol

local LevelComponent = Object:extend()

function LevelComponent:new()
  -- When dirty=True, forces the component to be updated (onUpdate() to be called during simulation).
  -- This way, you can update the component even if the inputs have not changed.
  -- Whenever the update is called, the dirty flag is automatically set to false again.
  self.dirty = true

  -- Local storage for the input/outputs of each wire.
  -- These are the temporary buffers that are used in input/output.
  --  prevIn=buffersPrevIn,
  --  nextIn=buffersNextIn,
  --  output=buffersOutput,
  -- check _updateBuffers() for more details
  self._buffers = nil

  -- Running flag to see if simulation is running.
  self.running = false
end

function LevelComponent:_updateBuffers()
  if self._buffers ~= nil then
    return self._buffers
  end
  -- uses pins definition to create the buffers
  local pins = self.pins
  -- The __order defines the order in which each wire appears.
  local buffersPrevIn = {__order={}}
  local buffersNextIn = {__order={}}
  local buffersOutput = {__order={}}
  for ip = 1,#pins do
    local pin = pins[ip]
    local ptype = pin[1]
    local plen = pin[2]
    local pname = pin[3]
    if ptype == 'input' then
      local dataNextIn = {}
      local dataPrevIn = {}
      for j = 1,plen do
        dataNextIn[j] = 0
        dataPrevIn[j] = 0
      end
      buffersPrevIn[pname] = dataNextIn
      buffersNextIn[pname] = dataPrevIn
      table.insert(buffersPrevIn.__order, pname)
      table.insert(buffersNextIn.__order, pname)
    else
      local data = {}
      for j = 1,plen do
        data[j] = 0
      end
      buffersOutput[pname] = data
      table.insert(buffersOutput.__order, pname)
    end
  end
  self._buffers = {
    prevIn=buffersPrevIn,
    nextIn=buffersNextIn,
    output=buffersOutput,
  }
  return self._buffers
end

-- Called at each clock cycle (rising edge).
-- It kinda tries to simulate a rising-edge flip flop behaviour.
-- The onClock is called BEFORE the image simulation with the new value of clock.
-- For example:
--   1. onTick is called for clock component.
--   2. clock component identifies a new clock cycle.
--   3. The clock component is updated (as in onUpdate()).
--   4. the onClock() is called at each level component. [it might flag an update in this component via .dirty=true]
--   5. A regular simulation is triggered on the image.
--      Simulation in image alternates between simulating image then
--      simulationg components, in that order.
--  This workflow guarantees that the synchronous clock to handle memory will
--  work on both the image and the components, since:
--    1. when image simulation is called, the component outputs are the outputs
--      from previous simulation, which will make flip flops work as intended.
--    2. when onClock is called on components, the inputs are the inputs from
--       previous simulation, so flip flops will also work as intended, however
--       the outputs will not change immediatly because the component outputs
--       only change in onUpdate but not on onClock.
--
-- Convention is that the clock is always the 1st component.
-- This doesnt work for the clock itself.
-- As argument, receives the last input of the component
--
-- The reset argument is a boolean indicating whether system is on "power on reset".
-- Normally, when reset=1, you only want to initiate the level and not process inputs.
--
function LevelComponent:onClock(nextIn, reset)
end

-- Called when simulation starts
-- Use it for initializing simulation stuff (rather than new()).
function LevelComponent:onStart()
end

-- Called when simulation stops
function LevelComponent:onStop()
end

-- Called for every simulation tick.
-- Not to confuse with clock tick or frame tick. The simulation tick is often
-- much higher than the clock itself, about 5khz. Normally you shouldn't need
-- it, and you should rely on onClock directly.
function LevelComponent:onTick(dt)
end

-- Called at every draw frame, used for drawing stuff on screen.
-- There's no convention on where each component should draw, so it's up to the
-- user to organize drawing acorss components. In most cases though, a single
-- component is used so that should be easy.
--
-- Parameters: rt is the overlay RenderTexture that is drawn on top of the
-- image. So most often you want to draw there. (you can also draw directly on
-- the screen). "cam" is a Camera object containing information about the
-- camera of the image. It's useful to draw on the same space as the image, so
-- it moves around when the user moves on the image. You can for instance draw
-- on the outter border of the image. Check camera.lua for more info.
function LevelComponent:onDraw(rt, cam)
end

-- This is the main component update function.
-- It is only called during simulation (it can be called multiple times if
-- input changes more than once).
-- It is called in 2 situations:
--   1. When an input changes.
--   2. The component dirty flag is set to true.
-- You normally don't want to change internal state in this function, for that
-- you should use onClock. It should behave more like a combinatorial /
-- calculation-only thing.
function LevelComponent:onUpdate(prevIn, nextIn, output)
end

-- Gets the value of a bit in a number
local function bget(n, ibit)
  local m = lshift(1, ibit)
  local r = rshift(band(n, m), ibit)
  return r
end

-- Sets the value of a bit in a number
function LevelComponent:setBits(arr, value)
  local blen = #arr
  for i = 1, blen do
    arr[i] = bget(value, i - 1)
  end
end

-- Helper function to convert a list of bits into a number
function LevelComponent:toNumber(v, ref)
  local out = 0
  for i = 1,#v do
    if v[i] == 1 then
      local m = lshift(1, i-1)
      out = bor(out, m)
    end
  end
  if ref ~= nil then
    if ref < 0 then
      local h1 = lshift(1, #v-1)
      local h2 = lshift(1, #v-0)
      -- 255 --> -1
      if out >= h1 then
        out = -(h2 - out)
      end
    end
  end
  return out
end


return LevelComponent
