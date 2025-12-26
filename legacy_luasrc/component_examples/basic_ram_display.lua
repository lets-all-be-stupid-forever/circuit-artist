local LevelComponent = require 'level_component'
local ffi = require 'ffi'
local Clock = require 'clock'
local utils = require 'utils'
local rl = require 'raylib_api'
local C = require 'c_api'

local BasicRAMDisplay = LevelComponent:extend()

local bget = utils.bget

-- display_w must be a multiple of 8 !
function BasicRAMDisplay:new(memory, display_w, display_h)
  BasicRAMDisplay.super.new(self)
  self.pins = {
    {'input', 8, 'ram_display_offset'}, -- used for memory swap
  }
  self.memory = memory
  -- assuming that each memory address has 8 bits
  self.display_w = display_w
  self.display_h = display_h
  self.display_offset = 0
end


function BasicRAMDisplay:onClock(inputs)
  self.display_offset = self:toNumber(inputs.ram_display_offset)
end

function BasicRAMDisplay:onDraw(rt)
  -- Drawing using raylib workflow
  -- This rt parameter is a RenderTexure object that is drawn right above the
  -- image display.
  -- Be careful though: the ondraw is called for each component, so its up to
  -- the developper to manage the different components drawing not to overlap!

  -- this func is called even when the sim is not running, so mind returning here
  if not self.running then
    return
  end

  local c2 = ffi.new('Color', {248,255,203,255})

  local black = ffi.new('Color', {0,0,0,255})
  local white = ffi.new('Color', {255,255,255,255})
  local blank = ffi.new('Color', {0,0,0,0})

  local mem = self.memory
  rl.BeginTextureMode(rt)
  rl.ClearBackground(blank)
  local tw = rt.texture.width
  local dw = self.display_w
  local dh = self.display_h
  local pixel_size = 10
  local pad = 20

  rl.rlPushMatrix()
  rl.rlTranslatef(tw - pad - dw * pixel_size, pad, 0)

  rl.DrawRectangle(-5, -5, dw*pixel_size +10, dh*pixel_size +10, c2)
  rl.DrawRectangle(0, 0, dw*pixel_size , dh*pixel_size , black)

  local size = dw * dh
  local size_bytes = size/8
  local off_bytes = self.display_offset + 1
  for y=0,dh-1 do
    local dw_bytes = dw/8
    for x_bytes=0,dw_bytes-1 do
      local m = mem[off_bytes]
      off_bytes = off_bytes + 1
      local x = x_bytes  * 8
      -- I'm not 100% confident the display here is correct, one might want to
      -- re-implement with a proper memory read order
      for ibit=0,7 do
        local bit = bget(m, ibit)
        local pixel_x = x*pixel_size
        local pixel_y = y*pixel_size
        if bit == 1 then
          pixel_color = white
        else
          pixel_color = black
        end
        rl.DrawRectangle(pixel_x, pixel_y, pixel_size, pixel_size, pixel_color)
        x = x + 1
      end
    end
  end
  rl.rlPopMatrix()
  rl.EndTextureMode()
end

return BasicRAMDisplay
