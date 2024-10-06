local Tester = require 'tester'
local Clock = require 'clock'
local rl = require 'raylib_api'
local ffi = require 'ffi'
local utils = require 'utils'

local lut_7seg = {
    {1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 0, 0, 0, 0},
    {1, 1, 0, 1, 1, 0, 1},
    {1, 1, 1, 1, 0, 0, 1},
    {0, 1, 1, 0, 0, 1, 1},
    {1, 0, 1, 1, 0, 1, 1},
    {1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1},
}

local SevenSeg = Tester:extend()

function SevenSeg:new()
  SevenSeg.super.new(self)
  self.sprites = rl.LoadTexture("../luasrc/imgs/levels/seven_seg_display.png")
  self.has_submit = false
  self.pins = {
    {'output', 4, 'n'},
    {'input', 1, "top"},
    {'input', 1, "upper_right"},
    {'input', 1, "lower_right"},
    {'input', 1, "bottom"},
    {'input', 1, "lower_left"},
    {'input', 1, "upper_left"},
    {'input', 1, "middle"},
  }
  self.schedule = {
    {n=0, top=1, upper_right=1, lower_right=1, bottom=1, lower_left=1, upper_left=1, middle=0, name='n=0'},
    {n=1, top=0, upper_right=1, lower_right=1, bottom=0, lower_left=0, upper_left=0, middle=0, name='n=1'},
    {n=2, top=1, upper_right=1, lower_right=0, bottom=1, lower_left=1, upper_left=0, middle=1, name='n=2'},
    {n=3, top=1, upper_right=1, lower_right=1, bottom=1, lower_left=0, upper_left=0, middle=1, name='n=3'},
    {n=4, top=0, upper_right=1, lower_right=1, bottom=0, lower_left=0, upper_left=1, middle=1, name='n=4'},
    {n=5, top=1, upper_right=0, lower_right=1, bottom=1, lower_left=0, upper_left=1, middle=1, name='n=5'},
    {n=6, top=1, upper_right=0, lower_right=1, bottom=1, lower_left=1, upper_left=1, middle=1, name='n=6'},
    {n=7, top=1, upper_right=1, lower_right=1, bottom=0, lower_left=0, upper_left=0, middle=0, name='n=7'},
    {n=8, top=1, upper_right=1, lower_right=1, bottom=1, lower_left=1, upper_left=1, middle=1, name='n=8'},
    {n=9, top=1, upper_right=1, lower_right=1, bottom=1, lower_left=0, upper_left=1, middle=1, name='n=9'},
  }
end
--
function drawSegment(sprites, i, x, y, c)
  local seg_width = sprites.width / 8
  local seg_height = sprites.height
  local source = ffi.new('Rectangle', {i * seg_width, 0, seg_width, seg_height})
  local target = ffi.new('Rectangle', {x, y, source.width, source.height})
  local origin = ffi.new('Vector2', {0})
  utils.rlDrawTexturePro(sprites, source, target, origin, 0, c);
end

function SevenSeg:customDraw(rt)
  local tw = rt.texture.width
  local th = rt.texture.height
  local c0 = ffi.new('Color', {108,108,83,255})
  local c1 = ffi.new('Color', {187,194,156,255})
  local c2 = ffi.new('Color', {248,255,203,255})

  rl.rlPushMatrix()
  rl.rlScalef(1, 1, 1)
  local black = ffi.new('Color', {0,0,0,255})
  local white = ffi.new('Color', {255,255,255,255})
  local w = 16 + 10 + 2
  rl.rlTranslatef(-w - 5, 0, 0)
  rl.DrawRectangle(0, 0, 16 + 10 + 2, 23 + 10 + 2, c0)
  rl.DrawRectangle(1, 1, 16 + 10, 23 + 10, black)
  rl.rlTranslatef(6,6,0)
  local gray = white
  gray.a = 10
  for i=1,8 do
    local colorGray = ffi.new('Color', {255, 255, 255, 10})
    drawSegment(self.sprites, i-1, 0, 0, colorGray)
  end
  for i=1,7 do
    local pname  = self.pins[i+1][3]
    if self._buffers.nextIn[pname][1] == 1 then
      local colorBlue = ffi.new('Color', {  102, 191, 255, 255 })
      drawSegment(self.sprites, i-1, 0, 0, colorBlue)
    end
  end
  rl.rlPopMatrix()
end

addLevel({
    icon = "../luasrc/imgs/levels/seven_seg_icon.png",
    name = "Seven Segments",
    desc=[[

!img:imgs/levels/seven_seg_img1.png

Display a digit in a seven-segment display.

The input is a 4-bit number `n` between 0 and 9.

You have 7 output bits, each corresponding to a segment in a 7-segment display. Your task is to activate the segments such that the display match the image above.

For validation, each number between 0 and 9 will be tested.

]],
    chips = {
      Clock(true),
      SevenSeg(),
    },
    id='7SEG',
    unlockedBy=nil,
})
