local LevelComponent = require 'level_component'
local Clock = require 'clock'
local rl = require 'raylib_api'

local BasicKeyboard = LevelComponent:extend()

function BasicKeyboard:new()
  BasicKeyboard.super.new(self)
  self.pins = {
    {'output', 1, 'key_w'},
    {'output', 1, 'key_a'},
    {'output', 1, 'key_s'},
    {'output', 1, 'key_d'},
  }
end

function BasicKeyboard:onStart()
  self.keys = {w=0,a=0,s=0,d=0}
end

-- Just returns the values defined on the current test case.
function BasicKeyboard:onUpdate(prevIn, nextIn, output)
  output.key_w[1] = self.keys.w
  output.key_a[1] = self.keys.a
  output.key_s[1] = self.keys.s
  output.key_d[1] = self.keys.d
end

local function toBit(bool)
  if bool then return 1 else return 0 end
end

function BasicKeyboard:onTick(dt)
  self.keys.w = toBit(rl.IsKeyDown(rl.KEY_W))
  self.keys.a = toBit(rl.IsKeyDown(rl.KEY_A))
  self.keys.s = toBit(rl.IsKeyDown(rl.KEY_S))
  self.keys.d = toBit(rl.IsKeyDown(rl.KEY_D))
  self.dirty = true
end

return BasicKeyboard
