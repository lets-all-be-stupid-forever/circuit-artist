local LevelComponent = require 'level_component'
local Clock = require 'clock'
local Hanoi = LevelComponent:extend()
local rl = require 'raylib_api'
local C = require 'c_api'
local ffi = require 'ffi'

function Hanoi:new(size)
  Hanoi.super.new(self)
  self.pins = {
    {'input', 1, 'action'},
    {'input', 3, 'src'},
    {'input', 3, 'dst'},
  }
  self.n = size
end

function Hanoi:onStart()
  local t = {}
  for i=self.n,1,-1 do
    table.insert(t, i)
  end
  self.state = {
    t, -- 1
    {}, -- 2
    {}, -- 3
  }
  self.result = 0
  self.errmsg = nil
end

function getPosition(arr)
  if arr == 1 then
    return 1
  elseif arr == 2 then
    return 2
  elseif arr == 4 then
    return 3
  else
    return -1
  end
end

function Hanoi:onClock(inputs, reset)
  if reset then
    return
  end
  if self.result ~= 0 then
    return
  end
  if inputs.action[1] ~= 1 then
    return
  end
  local src = getPosition(self:toNumber(inputs.src))
  local dst = getPosition(self:toNumber(inputs.dst))

  if src < 0 then
    self.errmsg = 'Error: Invalid coordinate in src: ' .. src .. ' , expected a single active bit.'
    self.result = -1
    stopClock()
    return
  end

  if dst < 0 then
    self.errmsg = 'Error: Invalid coordinate in dst: ' .. src .. ' , expected a single active bit.'
    self.result = -1
    stopClock()
    return
  end

  local tsrc = self.state[src]
  local tdst = self.state[dst]
  if #tsrc == 0 then
    self.errmsg = "Invalid move: can't move piece because origin tower (" .. src .. ") is empty."
    self.result = -1
    stopClock()
    return
  end
  local moving = tsrc[#tsrc]
  if #tdst > 0 and tdst[#tdst] < moving then
    self.errmsg = "Invalid move: destination tower's piece is bigger than the moving piece."
    self.result = -1
    stopClock()
    return
  end
  tsrc[#tsrc] = nil
  tdst[#tdst + 1] = moving
  -- movement succeeded
  if #self.state[1] == 0 and #self.state[2] == 0 then
    self.result = 1
    notifyLevelCompleted()
    stopClock()
  end
end

function Hanoi:onDraw(rt, cam, disp1)
  local black = ffi.new('Color', {0,0,0,255})
  local white = ffi.new('Color', {255,255,255,255})
  local color = ffi.new('Color', {55,55,0,255})
  local blank = ffi.new('Color', {0,0,0,0})
  local c2 = ffi.new('Color', {248,255,203,255})


  rl.BeginTextureMode(rt)
  -- cam:setup()
  rl.ClearBackground(blank)
  local tw = rt.texture.width
  if self.running then

    rl.rlPushMatrix()
    rl.rlScalef(2,2,1)
    local rw = 200
    rl.rlTranslatef((tw - rw * 2)/2, 0, 0)
    -- rl.rlScalef(2, 2, 1);
    -- rl.DrawRectangle(0, 0, 200, 180, black)
    local tower_width = 50
    local piece_height = 5
    local y0 = 100
    local status = 'Running...'
    if self.result == -1 then
      status = 'Failure.'
    elseif self.result == 1 then
      status = 'Level Complete'
    end
    C.CaDrawTextBox(status, 40, 20, 80, c2)
    rl.DrawRectangle(30, y0, 3*tower_width, 5, c2)
    for i=1,3 do
      local x0 = tower_width/2 + tower_width * (i-1) + 30
      rl.DrawLine(x0, y0 - piece_height * (self.n + 2), x0, y0, c2)
      local tower = self.state[i]
      for j = 1, #tower do
        local p = tower[j]
        rl.DrawRectangle(x0 - 2*p - 1, y0 - j * piece_height, 4*p+1, piece_height, c2)
      end
    end
    if self.errmsg ~= nil then
      C.CaDrawTextBox(self.errmsg, 10, y0 + 20, 80, c2)
    end
    rl.rlPopMatrix()
  end
  rl.EndTextureMode()
end

addLevel({
  name="Hanoi Tower",
  icon="../luasrc/imgs/levels/hanoi_icon.png",
  desc=[[

!img:imgs/levels/hanoi_img2.png

The game consist of a board with 10 pieces and 3 towers (tower `001`, tower `010` and tower `100`). The pieces have different sizes, from 1 to 10. The pieces start stacked in increasing size at the leftmost tower (tower `001`), with the biggest piece in the bottom and the smallest at the top, as shown in the picture (with 4 pieces).

!img:imgs/levels/hanoi_img3.png

The objective is to move all pieces to the rightmost tower (tower `100`).

The only action you can make in this game is moving the top piece from one tower to the other, following the constraints:

1. You can only move one piece at a time.
2. A piece can never be placed on top of a smaller piece.
3. You can't move a piece from a tower to the same tower.
4. You can't move from a tower with no piece on it (ie empty tower).

The first output is an action flag bit `action`, describing when you want to perform a moving action. When the bit is 0, nothing happens. When the bit is 1, the top piece from tower `src` is moved to the top of tower `dst`. This means you can perform calculations between clock cycles and activate the action bit whenever you want to perform a movement.

For example, if you want to move from tower `001` to tower `010`, you should make `src=001` and `dst=010`.

Note that you don't have access to the current state of the game, you'll have to track it yourself, given that you know the initial position.

The game ends with success after the last movement that makes all pieces be placed in the rightmost tower (tower `100`).

Below one example of how to solve the game with 2 pieces:

!img:imgs/levels/hanoi_img1.png

  ]],
    chips = {
      Clock(),
      Hanoi(10),
    },
    id='HANOI',
})
