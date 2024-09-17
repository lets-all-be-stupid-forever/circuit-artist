-- Helper level component class for levels with pre-defined results.
local LevelComponent = require 'level_component'
local Tester = LevelComponent:extend()
local utils = require 'utils'
local rl = require 'raylib_api'
local C = require 'c_api'
local ffi = require 'ffi'

local texFlag = rl.LoadTexture('../luasrc/imgs/flag.png')
local texSuccess = rl.LoadTexture('../luasrc/imgs/success.png')

function Tester:new()
  Tester.super.new(self)
  -- If has_submit = false, then we don't have a clock and we want to check
  -- theu results at every rising edge. Most often that's the case of
  -- combinatorial circuit levels.
  -- However, if submit=true, we expect a pin with name "submit", and the
  -- results will only be checked whenever submit=1. This allows the user to
  -- perform calculations using multiple clock cycles.
  -- See default levels for more examples.
  self.has_submit = false
end

function Tester:onStart()
  -- Total number of tests
  self.total = #self.schedule
  -- Current test number
  self.curi = 1
  -- Current result:
  --  0 -> in progress / running
  --  1 -> success
  --  -1 -> failure
  self.result = 0
  -- List of errors (for the case result=-1)
  self.errors = {}
  -- Number of clocks taken to solve the problem
  self.clocks_taken = 0
end

local function makeTestPage(pins, s, test_number)
  local r = 'Test ' .. test_number .. '\n\n'
  for i=1, #pins do
    local pname = pins[i][3]
    if s[pname] ~= nil then
      r = r .. pname .. '=' .. s[pname] .. '\n'
    end
  end
  return r
end

-- experimental
function Tester:createPages()
  local pages = {}
  for i=1,#self.schedule do
    table.insert(pages, {
      name='t' .. i,
      content=makeTestPage(self.pins, self.schedule[i], i),
    })
  end
  return pages
end

local function isValid(n)
  for i=1,#n do
    if n[i] ~= 0 and n[i] ~= 1 then
      return false
    end
  end
  return true
end

function Tester:onClock(inputs)
  -- If test is complete/failed, stop here.
  if self.result ~= 0 then
    return
  end
  -- If it has submit and the submit flag is not 1, then stop.
  if self.has_submit and inputs.submit[1] ~= 1 then
    return
  end
  -- Current test case table
  local s = self.schedule[self.curi]
  local error_msg = nil
  -- for each input pin, will check if the value correspond to the desired
  -- results in the test case when applicable (some input pins can be ignored)
  for _, name in ipairs(inputs.__order) do
    -- Desired result
    local ans = s[name]
    -- If desired result is not nil, procceed with test
    if ans ~= nil then
      -- Convert input bits to a number so we can check
      if not isValid(inputs[name]) then
        table.insert(self.errors, {name=name, expected=ans, result='undefined'})
      end
      local r = self:toNumber(inputs[name])
      -- checks if returned value (r) is different of the desired answer (ans),
      -- in which case, an error is created.
      if r ~= ans then
        -- An error item iscreated with the name of the pin that failed, the
        -- expected and returned value.
        table.insert(self.errors, {name=name, expected=ans, result=r})
      end
    end
  end

  -- if any error was created, then we declare the test as failed and stop the
  -- clock.
  if #self.errors > 0 then
    self.result = -1
    stopClock()
    return
  end

  -- However, if no error has been raised, we avance to the next test case.
  self.curi = self.curi + 1
  -- If we have been through all the tests cases, then we declare the test as successful and stop the clock.
  if self.curi == #self.schedule + 1 then
    self.result = 1
    self.clocks_taken = stopClock()
    return
  end
  -- Here, the test is still running, (ie not failed nor success), and we set
  -- the component to dirty so the level can send the next test case values  on
  -- the next simulation update.
  self.dirty = true
end

-- Just returns the values defined on the current test case.
function Tester:onUpdate(prevIn, nextIn, output)
  if self.result == 0 then
    local s = self.schedule[self.curi]
    for _, name in ipairs(output.__order) do
      if s[name] ~= nil then
        self:setBits(output[name], s[name])
      end
    end
  end
end

function Tester:onDraw(rt, cam, disp1)
  local black = ffi.new('Color', {0,0,0,255})
  local black2 = ffi.new('Color', {0,0,0,150})
  local white = ffi.new('Color', {255,255,255,255})
  local white2 = ffi.new('Color', {255,255,255,200})
  local color = ffi.new('Color', {55,55,0,255})
  local green = ffi.new('Color', {0,117,44,255})
  local green2 = ffi.new('Color', {0,228,48,255})
  local yellow = ffi.new('Color', {253, 249, 0, 255})
  local red = ffi.new('Color', {230, 41, 55, 255 })
  local dpurp = ffi.new('Color', {112, 31, 126, 255 })
  local blank = ffi.new('Color', {0,0,0,0})
  local c0 = ffi.new('Color', {108,108,83,255})
  local c1 = ffi.new('Color', {187,194,156,255})
  local c2 = ffi.new('Color', {248,255,203,255})
  local tw = rt.texture.width
  local th = rt.texture.height
  rl.BeginTextureMode(rt)
  rl.ClearBackground(blank)
  rl.rlPushMatrix()
  rl.rlScalef(1,1,1)
  if self.running then
    local msgs = {}
    local tw = rt.texture.width
    rl.rlPushMatrix()
    rl.rlScalef(2,2,1)
    if self.result == 1 then
      -- table.insert(msgs, {text='Success (' .. self.clocks_taken .. ' clocks)', color=green})
      table.insert(msgs, {text='Success', color=green})
    elseif self.result == -1 then
      table.insert(msgs, {text='Failure', color=red})
    else
      table.insert(msgs, 'Running...')
    end
    if self.result == 0 or self.result == -1 then
      table.insert(msgs, 'Test ' .. self.curi .. '/' .. #self.schedule)
      table.insert(msgs, self.schedule[self.curi].name)
    end
    if #self.errors > 0 then
      for _, err in ipairs(self.errors) do
        table.insert(msgs, 'Expected: ' .. err.name .. '=' .. err.expected)
        table.insert(msgs, 'Returned: ' .. err.name .. '=' .. err.result)
        break
      end
    end
    rl.rlPushMatrix()
    rl.rlTranslatef(0, 10, 0)
    local p = 5
    local lh = 14;
    for i=1,#msgs do
      local txt = msgs[i]
      local bg = black
      local border = false
      if type(txt) == 'table' then
        local t = txt
        txt = t.text
      end
      local tx = C.CaGetDrawTextSize(txt)
      local w = tx + 2*p
      local kk = tw/2 - w - 5
      rl.rlTranslatef(kk, 0, 0)
      rl.DrawRectangleLines(0, 0, w, lh+2*p, c0)
      rl.DrawRectangle(1, 1, w-2, lh+2*p - 2, bg)
      if border then
        C.CaDrawText(txt, p+1, 4+p+1, black)
      end
      C.CaDrawText(txt, p, 4+p, c2)
      rl.rlTranslatef(-kk, 0, 0)
      rl.rlTranslatef(0, lh + 2*p + 2, 0)
    end
    rl.rlTranslatef(tw/2, 0, 0)
    if self.customDraw then
      self:customDraw(rt)
    end
    rl.rlPopMatrix()
    rl.rlPopMatrix()

    rl.rlPushMatrix()
    rl.rlTranslatef(tw, th, 0)
    local nt = #self.schedule
    local nx = math.ceil(math.sqrt(nt))
    local ny = nx
    if nx < 8 then
      rl.rlScalef(2,2,1)
    end
    local rw = 13
    local rh = 13
    local xx0 = - rw*nx -  5
    local yy0 = - rh*ny -  5
    for ty=0,ny-1 do
      for tx=0,nx-1 do
        local xx = xx0 + tx*rw
        local yy = yy0 + ty*rh
        local ct = ty * nx + tx + 1
        if ct <= nt then
          local bg = black
          local fg = white
          if ct == self.curi then
            if self.result == 0 then
              bg = yellow
              fg = black
            elseif self.result == -1 then
              bg = red
            end
          elseif ct < self.curi then
            if self.result == 1 then
              bg = green2
              fg = black
            else
              bg = green
            end
          end
          rl.DrawRectangle(xx, yy, rw, rh, bg)
          rl.DrawTexture(texFlag, xx, yy, fg)
        end
      end
    end
    if self.result == 1 then
       rl.DrawRectangle(xx0, yy0, rw*nx, rh*ny, black2)
       local src = ffi.new('Rectangle', {0, 0, texSuccess.width, texSuccess.height})
       local dst = ffi.new('Rectangle', {xx0, yy0, rw*nx, rh*ny})
       local ori =  ffi.new('Vector2', {0, 0})
       utils.rlDrawTexturePro(texSuccess, src, dst, ori, 0, c2)
    end

    rl.rlPopMatrix()

  end
  rl.rlPopMatrix()
  rl.EndTextureMode()
end

return Tester
