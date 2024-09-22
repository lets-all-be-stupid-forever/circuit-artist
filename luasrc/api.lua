local ffi = require 'ffi'
local R = require 'raylib_api'
local C = require 'c_api'
local help = require 'ctpd'
local Camera = require 'camera'
local textbox = require 'textbox'

local S = C.GetSharedState()
local free = ffi.C.free
local malloc = ffi.C.malloc
local cast = ffi.cast
local sizeof = ffi.sizeof

-- Level registry
local levelOptions = {}

function getLevelComponents()
  return levelOptions[S.level_desc.ilevel+1].chips
end

function getLevel()
  return levelOptions[S.level_desc.ilevel+1]
end

-- Copies a int* bits to the internal buffer space in a level
-- Used only in input bits
local function copyInputBits(srcPtr, dstBuffer)
  local c = 0
  for _, name in ipairs(dstBuffer.__order) do
    local v = dstBuffer[name]
    for j = 1, #v do
        v[j] = srcPtr[c]
        c = c + 1
    end
  end
end

local function copyOutputBits(srcBuffer, dstPtr)
  local c = 0
  for _, name in ipairs(srcBuffer.__order) do
    local v = srcBuffer[name]
    for j = 1, #v do
        dstPtr[c] = v[j]
        c = c + 1
    end
  end
end

local function toSprite(img)
  if type(img)== 'string' then
    local tex = R.LoadTexture(img)
    img = { tex, 0, 0, tex.width, tex.height, }
  end

  local s = ffi.new('Sprite')
  s.tex = img[1]
  local x = img[2]
  local y = img[3]
  local w = img[4]
  local h = img[5]
  s.region = ffi.new('Rectangle', {x,y,w,h})
  return s
end

function initCircuitopedia()
  for ih=1,#help do
    local h = help[ih]
    S.level_options.help_name[ih-1] = h.name
    local t = textbox.prepareText(h.text)
    -- Hack to make lua keep the table with the string so the reference in C is
    -- still valid.
    -- If we don't do this, the string will be garbage collected and the C
    -- pointer will be invalidated
    -- Ideally, we would like to do a strdup here, but idk how to do it :(
    h._t = t
    S.level_options.help_txt[ih-1] = t.text
    for i=1,#t.sprites do
      S.level_options.help_sprites[ih-1][i-1] = t.sprites[i]
    end
  end
end

function addLevel(co)
  table.insert(levelOptions, co)
  local i = #levelOptions
  print('added level ' .. i .. ' ' .. co.name)
  -- local co = levelOptions[i]
  local opt = S.level_options.options[i-1]
  opt.ilevel = i-1
  local icon = R.LoadTexture(co.icon)
  opt.icon = toSprite({icon, 0, 0, 33, 33})
  local t = textbox.prepareText(co.desc)
  co._t = t
  -- Hack to make lua keep the table with the string so the reference in C is
  -- still valid.
  -- If we don't do this, the string will be garbage collected and the C
  -- pointer will be invalidated
  opt.desc = t.text
  for k=1,#t.sprites do
    opt.sprites[k-1] = t.sprites[k]
  end
  opt.name = co.name
  return i
end

function initApp()
  initCircuitopedia()
end

local function setDirtyFlag(i, value)
  S.level_desc.extcomps[i-1].dirty = true
end

local function setLevelsDirty()
  local nc = S.level_desc.num_components
  for i =1,nc do
    setDirtyFlag(i, true)
  end
end

local function setApiClockValueAndCount(clock_value, clock_count, por)
  S.clock_update_value = clock_value
  S.clock_count = clock_count
  S.por = por
end

local function unloadStateLevels()
  local cd = S.level_desc
  if cd.pindesc ~= nil then
    free(cd.pindesc)
    cd.pindesc = nil
  end
  if cd.extcomps ~= nil then
    free(cd.extcomps)
    cd.extcomps = nil
  end
  cd.num_components = 0
end

function setInitialLevelByName(levelName)
  for i=1,#levelOptions do
    if levelOptions[i].name == levelName then
        S.requested_level = i-1
    end
  end
end

-- Called by C but can also be called from lua
function apiLoadLevel()
  unloadStateLevels()
  ilevel = S.requested_level
  local cd = S.level_desc
  cd.ilevel = ilevel
  local co = levelOptions[ilevel+1]
  local nc = #co.chips
  cd.num_components = nc
  cd.pindesc = malloc(sizeof('PinDesc') * nc)
  cd.extcomps = malloc(sizeof('ExtComp') * nc)
  local y = 0
  for i = 1, nc do
    local comp = co.chips[i]
    local pins = comp.pins
    local pd = cd.pindesc[i-1]
    pd.num_conn = #pins
    local ec= cd.extcomps[i-1]
    ec.ni = 0
    ec.dirty = true
    ec.no = 0
    for j = 1, #pins do
      local pin = pins[j]
      local len = pin[2]
      pd.conn[j-1].len = len
      pd.conn[j-1].name = pin[3]
      if pin[1] == 'input' then
        pd.conn[j-1].type = 0
        for k = 1, len do
          ec.wires_in_x[ec.ni] = 0
          ec.wires_in_y[ec.ni] = y + 4
          ec.ni = ec.ni + 1
          y = y + 2
        end
      else
        pd.conn[j-1].type = 1
        for k = 1, len do
          ec.wires_out_x[ec.no] = 0
          ec.wires_out_y[ec.no] = y + 4
          ec.no = ec.no + 1
          y = y + 2
        end
      end
      y = y + 6
    end
  end
end

-- Called by C
function apiStartSimulation()
  setLevelsDirty()
  local comps = getLevelComponents()
  for i = 1, #comps do
    comps[i].running = true
    comps[i]:onStart()
    S.level_desc.extcomps[i-1].dirty = true
  end
end

-- Called by C
function apiStopSimulation()
  local comps = getLevelComponents()
  for i = 1, #comps do
    comps[i].running = false
    comps[i]:onStop()
  end
end

-- Called by C
function apiOnLevelTick()
  local comps = getLevelComponents()
  for i = 1, #comps do
    comps[i].dirty = S.level_desc.extcomps[i-1].dirty
    comps[i]:onTick(S.dt)
    S.level_desc.extcomps[i-1].dirty = comps[i].dirty
    if i == 1 then
      local level = comps[1]
      local clock_value = -1
      local por = level:getPowerOnReset()
      if level.dirty and por == 0 then
        clock_value = level.value
      end
      local clock_count = comps[i]:getClockCount()
      setApiClockValueAndCount(clock_value, clock_count, por)
    end
  end
end

local function getUpdateLevelComponent()
  local comps = getLevelComponents()
  local icomp = S.update_ctx.ic + 1
  return icomp, comps[icomp]
end

-- Called by C
function apiOnLevelClock()
  local inputs = S.update_ctx.next_in
  local icomp, level = getUpdateLevelComponent()
  local buffers = level:_updateBuffers()
  local reset = S.update_ctx.reset
  copyInputBits(inputs, buffers.nextIn)
  level.dirty = S.level_desc.extcomps[icomp-1].dirty
  level:onClock(buffers.nextIn, reset)
  S.level_desc.extcomps[icomp-1].dirty = level.dirty
end

-- Called by C
function apiUpdateLevelComponent()
  local icomp, level = getUpdateLevelComponent()
  local buffers = level:_updateBuffers()
  local nextIn = buffers.nextIn
  local prevIn = buffers.prevIn
  local output = buffers.output
  copyInputBits(S.update_ctx.next_in, nextIn)
  copyInputBits(S.update_ctx.prev_in, prevIn)
  copyInputBits(S.update_ctx.output, output)
  level.dirty = S.level_desc.extcomps[icomp-1].dirty
  level:onUpdate(prevIn, nextIn, output)
  S.level_desc.extcomps[icomp-1].dirty = level.dirty
  copyOutputBits(output, S.update_ctx.output)
end

-- Called by C
function apiOnLevelDraw()
  local rt = S.rt
  local cam = Camera(S.cx, S.cy, S.cs)
  local comps = getLevelComponents()
  for icomp=1,#comps do
    comps[icomp]:onDraw(rt, cam)
  end
end

function initPaletteFromImage(path)
  local img = R.LoadImage(path)
  C.CaSetPalFromImage(img)
end

-- Sets initial image of the game.
-- It's useful when you're developping a level so you don't need
-- to re-open your image every time.
function setStartupImage(image_path)
  C.CaSetStartupImage(image_path)
end

