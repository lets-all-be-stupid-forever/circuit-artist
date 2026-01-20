local __ctx = {
   variables = {},
   changes = {},
}

Component = Object:extend()

local __chips = {}

function add_chip_instance(chip)
  table.insert(__chips, chip)
end

local function fixpins(ports, x)
  local y = 0
  for i=1,#ports do
    local w = ports[i].width
    ports[i].y0 = y
    y = y + 4
    local pins = {}
    for p=1,w do
      pins[p] = {x=x, y=y}
      y = y + 2
    end
    y = y + 2
    ports[i].pins = pins
  end
end

local function fixpins_v2()
  local y = 0
  local lports = {}
  local rports = {}
  for i=1,#PORTS do
    if PORTS[i].input then
      table.insert(rports, PORTS[i])
    else
      table.insert(lports, PORTS[i])
    end
  end
  fixpins(lports, 0)
  fixpins(rports, -1)
end

function var(v0)
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

function resetVariables()
  __ctx.variables = {}
end

function commit()
  local out = __ctx.changes
  __ctx.changes = {}
  if #out == 0 then
    return nil
  else
    return out
  end
end


function _forward(patch)
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

function _backward(patch)
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

function _setup(args)
  for i=1,#__chips do
    __chips[i]:setup()
    local ports = __chips[i].ports
    for j=1,#ports do
      local p = ports[j]
      local ptype
      if p.input then
        AddPortIn(p.width, p.name)
      else
        AddPortOut(p.width, p.name)
      end
    end
  end
end


function _start()
  resetVariables()
  for i=1,#__chips do
    __chips[i]:start()
  end
end

function _update()
  for i=1,#__chips do
    __chips[i]:update()
  end
  return commit()
end

function _draw()
  for i=1,#__chips do
    if __chips[i].draw ~= nil then
      __chips[i]:draw()
    end
  end
end

print('LOADED PRELOAD')
