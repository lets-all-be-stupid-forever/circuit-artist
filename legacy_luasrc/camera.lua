local Object = require "classic"
local rl = require 'raylib_api'
local Camera = Object:extend()

function Camera:new(x, y, s)
  self.x = x
  self.y = y
  self.s = s
end

function Camera:setup()
  rl.rlTranslatef(self.x, self.y, 0);
  rl.rlScalef(self.s, self.s, 1);
end

return Camera
