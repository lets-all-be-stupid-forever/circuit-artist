--[[
Mini framework for sequential tests.
]]

local __ctx = {
   variables = {},
   changes = {},
}

function Var(v0)
  local idx = #__ctx.variables + 1
  __ctx.variables[idx] = v0
  return function(arg)
    if arg == nil then
      return __ctx.variables[idx]
    else
      local before = __ctx.variables[idx]
      local after = arg
      table.insert(__ctx.changes, {idx, before, after})
    end
  end
end

function ResetVariables()
  __ctx.variables = {}
end

function CommitVariables()
  local out = __ctx.changes
  __ctx.changes = {}
  if #out == 0 then
    return nil
  else
    return out
  end
end

function ForwardVariables(patch)
  if patch == nil then
    return
  end
  for i=1,#patch do
    local desc = patch[i]
    local idx = desc[1]
    local v1 = desc[3]
    __ctx.variables[idx] = v1
  end
end

function BackwardVariables(patch)
  if patch == nil then
    return
  end
  for i=1,#patch do
    local desc = patch[i]
    local idx = desc[1]
    local v0 = desc[2]
    __ctx.variables[idx] = v0
  end
end

function bits(s)
  if s[1] == '2' then
    return nil
  else
    return tonumber(s, 2)
  end
end

function _Setup()
  PORT_POR = AddPortOut(1, 'power_on_reset', LEFT)
  PORT_CLK = AddPortOut(1, 'clock', LEFT)
  PORT_SUBMIT = AddPortIn(1, 'submit', LEFT)
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

function _Start()
  cycle = 0
  v_icase = Var(1)
  v_done = Var(false)
  v_err = Var(false)
  v_errors = Var(nil)
end

local function fixValue(v)
  if type(v) == 'number' then
    return v
  end
  if type(v) == 'string' then
    return bits(v)
  end
end


local function WriteCase(icase)
  for iport=1,#ports do
    local port = ports[iport]
    if port.dir == 'in' then
      local value = cases[icase][port.name]
      WritePort(port.addr, fixValue(value))
    end
  end
end

-- cycle 1 is the first risingn edge
local function IsRisingEdge()
  return (cycle % 2) == 1
end

local function WriteClock()
  WritePort(PORT_CLK, cycle % 2)
  if cycle == 0 then
    WritePort(PORT_POR, 1)
  end
  -- Changes por on first falling edge
  if cycle == 2 then
    WritePort(PORT_POR, 0)
  end
end

-- Returns a list with all the errors, or empty list if result matched
function CheckResults(icase)
  local errors = {}
  for iport=1,#ports do
    local port = ports[iport]
    if port.dir == 'out' then
      -- Testing
      local expect = cases[icase][port.name]
      local expectValue = fixValue(expect)
      local output = ReadPort(port.addr)
      if expectValue ~= nil and expectValue ~= output then
        table.insert(errors, {
            port=port.name,
            expected=expect,
            output=output,
        })
      end
    end
  end
  return errors
end


function _Update()
  local err = v_err()
  local done = v_done()
  if err or done then
    return
  end
  if cycle == 1 then
    WriteCase(1)
  end

  local icase = v_icase()
  local submit = ReadPort(PORT_SUBMIT)
  local rising = IsRisingEdge()

  if submit == 1 and cycle >= 3 and not rising then
    local errors = CheckResults(icase)
    if #errors > 0 then
      -- test failed
      v_err(true)
      v_errors(errors)
      Pause()
    else
      if icase == #cases then
        v_done(true)
        Pause()
      else
        WriteCase(icase + 1)
        v_icase(icase + 1)
      end
    end
  end
  WriteClock()
  return CommitVariables()
end

function _Forward(patch)
  cycle = cycle + 1
  ForwardVariables(patch)
end

function _Backward(patch)
  cycle = cycle - 1
  BackwardVariables(patch)
end

local GREEN = {0, 255, 0, 255}
local RED = {255, 0, 0, 255}
local BLACK = {0, 0, 0, 255}
local BLUE = {0, 128, 255, 255}
local WHITE = {248, 255, 203, 255}
local WHITE_ = {255, 255, 255, 255}

function _Draw()
  local errors = v_errors()
  local done = v_done()
  local err = v_err()
  local icase = v_icase()
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
    errors = errors[1]
    table.insert(msgs, {text='Expected:', color=RED})
    table.insert(msgs, {text=errors.port .. '=' .. errors.expected})
    table.insert(msgs, {text='Got:', color=RED})
    table.insert(msgs, {text=errors.port .. '=' .. errors.output})
  end

  -- Then is the current test index
  if not done and icase > 0 then
    local numCases = #cases
    table.insert(msgs, {text='Test ' .. icase .. '/' .. numCases})
    local cname = cases[icase].name
    if cname == nil then
      cname = ''
      for i=1, #ports do
        local port = ports[i]
        if port.dir == 'in' then
          cname = cname .. port.name  .. '=' .. cases[icase][port.name] .. ' '
        end
      end
    end
    table.insert(msgs, {text=cname, box_w=300})
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


