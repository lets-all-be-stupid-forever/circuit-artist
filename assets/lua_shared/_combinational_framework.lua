--[[
Mini framework for running combinatorial tests.

Instead of having to write the full script workflow, you can just define the
interfaces and input/outputs, import this file and the game will test your
circuit as is done in some campaign levels.
]]

function bits(s)
  if s[1] == '2' then
    return nil
  else
    return tonumber(s, 2)
  end
end


function _Setup()
  for i=1, #ports do
    local port = ports[i]
    local name = port.name
    local width = port.width
    local side = LEFT
    if port.side ~= nil then
      if port.side == 'left' then
        side= LEFT
      else
        side = RIGHT
      end
    end

    if port.dir == 'in' then
      port.addr = AddPortOut(width, name, side)
    else
      port.addr = AddPortIn(width, name, side)
    end
  end
  EnableRewind()
end

local function fixValue(v)
  if type(v) == 'number' then
    return v
  end
  if type(v) == 'string' then
    return bits(v)
  end
end

function _Start()
  -- Current case being tested (index)
  icase = 0
  -- whether it has completed
  done = false
  -- flag for an error
  err = false
  -- an object describing the error (so we can display in
  -- _Draw())
  errors = nil
end

function _Update()
  local patch = {}
  if err or done then
    return patch
  end
  if icase > 0 then
    for iport=1,#ports do
      local port = ports[iport]
      if port.dir == 'out' then
        -- Testing
        local expect = cases[icase][port.name]
        local expectValue = fixValue(expect)
        local output = ReadPort(port.addr)
        if expectValue ~= nil and expectValue ~= output then
          patch.err = true
          patch.errors = {
              port=port.name,
              expected=expect,
              output=output,
          }
          -- Pauses game on error (the first time)
          Pause()
          return patch
        end
      end
    end
    if icase == #cases then
      patch.done = true
      -- notify_level_complete() Used only in campaign (for now)
      Pause()
      return patch
    end
  end
  -- Now dispatches result of next case
  for iport=1,#ports do
    local port = ports[iport]
    if port.dir == 'in' then
      local value = cases[icase+1][port.name]
      WritePort(port.addr, fixValue(value))
    end
  end
  patch.icase = icase+1
  return patch
end

-- Little trick to replace a value in the patch object by it's
-- current global value and the new value.
function WrapPatch(func)
  return function()
    local patch = func()
    local out = {}
    for key, value in pairs(patch) do
      out[key] = {_G[key], value}
    end
    return out
  end
end
_Update = WrapPatch(_Update)

-- Very simple (beforeValue, afterValue) patch function
function _Forward(patch)
  for key, value in pairs(patch) do
    _G[key] = value[2]
  end
end

function _Backward(patch)
  for key, value in pairs(patch) do
    _G[key] = value[1]
  end
end

local GREEN = {0, 255, 0, 255}
local RED = {255, 0, 0, 255}
local BLACK = {0, 0, 0, 255}
local BLUE = {0, 128, 255, 255}
local WHITE = {248, 255, 203, 255}
local WHITE_ = {255, 255, 255, 255}

function _Draw()
  local msgs = {}
  -- First the general status message
  if done then
    table.insert(msgs, {text='Level Complete', color=GREEN})
  elseif err then
    table.insert(msgs, {text='Failure', color=RED})
  else
    table.insert(msgs, {text='Running...', color=BLUE})
  end
  if errors ~= nil then
    table.insert(msgs, {text='Expected:', color=RED})
    table.insert(msgs, {text=errors.port .. '=' .. errors.expected})
    table.insert(msgs, {text='Got:', color=RED})
    table.insert(msgs, {text=errors.port .. '=' .. errors.output})
  end

  -- Then is the current test index
  if not done and icase > 0 then
    local numCases = #cases
    table.insert(msgs, {text='Test ' .. icase .. '/' .. numCases})
    table.insert(msgs, {text=cases[icase].name, box_w=300})
  end

  rlPushMatrix();
  rlScalef(3,3,1);
  for i=1,#msgs do
    local txt = msgs[i].text
    local color = msgs[i].color or WHITE
    local lh = 8
    local pady = 1
    local bh = lh + 2*pady
    local y = (i - 1) * bh + pady
    -- local size = MeasureText(txt, lh)
    if txt ~= nil then
      local bg = BLACK
      local w = MeasureText(txt)
      local off = 4
      DrawRectangle(0, y - pady, w + 6, bh, {0,0,0,200})
      local box_w = msgs[i].box_w
      if box_w ~= nil then
        DrawTextBox(txt, off + 1, y+1, box_w, bg)
        DrawTextBox(txt, off + 0, y, box_w, color)
      else
        DrawText(txt, off + 1, y+1, bg)
        DrawText(txt, off + 0, y, color)
      end
    end
  end
  rlPopMatrix();
end



