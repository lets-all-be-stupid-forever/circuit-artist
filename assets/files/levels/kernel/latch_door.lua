local ASSET_BASE = 0
local ASSET_OPEN = 1
local ASSET_CLOSED = 2
local ASSET_LOCK_UP = 3
local ASSET_LOCK_DOWN = 4
local ASSET_UNLOCK_UP = 5
local ASSET_UNLOCK_DOWN = 6


local function customDraw(self)
  -- First I need access to pins
  local case = self:case()

  local w = 111
  local h = 122
  local r = {0, 0, w, h}
  local pins = {}

  local bg = {2, 2, 2, 255}
  local s = 2
  local c = {255, 255, 255, 255}

  local lock_up = false
  local unlock_up = false
  if case ~= nil then
    lock_up = case.lock_door_button
    unlock_up = case.unlock_door_button
  end

  local iunlock = 0
  local ilock = 0
  local iclosed = 0
  if unlock_up == 1 then
    iunlock = ASSET_UNLOCK_DOWN
  else
    iunlock = ASSET_UNLOCK_UP
  end
  if lock_up == 1 then
    ilock = ASSET_LOCK_DOWN
  else
    ilock = ASSET_LOCK_UP
  end
  if pget(2) == 1 then
    iclosed = ASSET_CLOSED
  else
    iclosed = ASSET_OPEN
  end



  local x0 = 800
  local y0 = 0
  rl_push_matrix();
  rl_translatef(x0,y0,0);
  rl_scalef(s,s,1);
  draw_rectangle_pro(0, 0, w, h, 0, 0, 0, bg[1], bg[2], bg[3], bg[4])
  draw_texture_pro(ASSET_BASE, 0, 0, w, h, 0, 0, w, h, 0, 0, 0, c[1], c[2], c[3], c[4])
  draw_texture_pro(iclosed, 0, 0, w, h, 0, 0, w, h, 0, 0, 0, c[1], c[2], c[3], c[4])
  draw_texture_pro(ilock, 0, 0, w, h, 0, 0, w, h, 0, 0, 0, c[1], c[2], c[3], c[4])
  draw_texture_pro(iunlock, 0, 0, w, h, 0, 0, w, h, 0, 0, 0, c[1], c[2], c[3], c[4])
  rl_pop_matrix()
end

local extra = {
  customDraw=customDraw
}

comb_level([[
conn out 1 unlock_door_button
conn out 1 lock_door_button
conn in 1 door_locked
test 0 0 2 Initial state: no button pressed
test 1 0 0 Unlock door button pressed: door should be unlocked. (door_locked=0)
test 0 0 0 Unlock door button released: door should remain unlocked. (door_locked=0)
test 1 0 0 Unlock door button pressed (again): door should be unlocked. (door_locked=0)
test 0 0 0 Unlock door button released: door should remain unlocked. (door_locked=0)
test 0 1 1 Lock door button pressed: door should be locked. (door_locked=1)
test 0 0 1 Lock door button released: door should remain locked. (door_locked=1)
test 0 1 1 Lock door button pressed (again): door should be locked. (door_locked=1)
test 0 0 1 Lock door button released: door should remain locked. (door_locked=1)
test 1 0 0 Unlock door button pressed: door should be unlocked. (door_locked=0)
test 0 0 0 Unlock door button released: door should remain unlocked. (door_locked=0)
test 0 1 1 Lock door button pressed: door should be locked. (door_locked=1)
test 0 0 1 Lock door button released: door should remain locked. (door_locked=1)
]], extra)
