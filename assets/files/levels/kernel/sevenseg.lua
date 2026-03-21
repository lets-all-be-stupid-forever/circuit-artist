Import "shared/comb_level.lua"


local seg_tex = LoadTexture("seven_seg_display.png")

local function customDraw(self)
  -- First I need access to pins
  local pins = {}
  for i =1,7 do
    pins[i] = ReadPort(i)
  end

  local white = {255, 255, 255, 255}
  local gray = {20, 20, 20, 255}
  local blue = {102, 191, 255, 255 }

  local bg = {2, 2, 2, 255}
  local border= {100, 100, 100, 255}

  -- w=128
  -- h=23
  local w = 128/8
  local h = 23

  local x0 = 10
  local y0 = 220
  local s = 3

  rlPushMatrix()
  rlTranslatef(x0, y0, 0)
  rlScalef(2,2,1)
  DrawRectanglePro({-2, -2, s*w+4, s*h+4}, {0, 0}, 0, border)
  DrawRectanglePro({0, 0, s*w, s*h}, {0, 0}, 0, bg)
  local function drawseg(k, c)
    local x = k * w
    DrawTexturePro(seg_tex,
      {x, 0, w, h},
      {0, 0, s*w, s*h},
      {0, 0},
      0,
      c
    )
  end

  for i=1,7 do
    drawseg(i-1, gray)
    if pins[i] == 1 then
      drawseg(i-1, blue)
    end
  end
  rlPopMatrix()


end


local extra = {
  customDraw=customDraw
}

comb_level([[
conn out 4 n
conn in 1 top
conn in 1 upper_right
conn in 1 lower_right
conn in 1 bottom
conn in 1 lower_left
conn in 1 upper_left
conn in 1 middle
test 0000 1 1 1 1 1 1 0 n=0
test 0001 0 1 1 0 0 0 0 n=1
test 0010 1 1 0 1 1 0 1 n=2
test 0011 1 1 1 1 0 0 1 n=3
test 0100 0 1 1 0 0 1 1 n=4
test 0101 1 0 1 1 0 1 1 n=5
test 0110 1 0 1 1 1 1 1 n=6
test 0111 1 1 1 0 0 0 0 n=7
test 1000 1 1 1 1 1 1 1 n=8
test 1001 1 1 1 1 0 1 1 n=9
]], extra)
