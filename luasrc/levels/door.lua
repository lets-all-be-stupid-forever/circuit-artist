local Tester = require 'tester'
local Clock = require 'clock'
local rl = require 'raylib_api'
local ffi = require 'ffi'
local utils = require 'utils'
local Door = Tester:extend()

function Door:new()
  Door.super.new(self)
  self.sp_base = rl.LoadTexture("../luasrc/imgs/levels/door_base.png")
  self.sp_open = rl.LoadTexture("../luasrc/imgs/levels/door_open.png")
  self.sp_closed = rl.LoadTexture("../luasrc/imgs/levels/door_closed.png")
  self.sp_lock_up= rl.LoadTexture("../luasrc/imgs/levels/door_lock_up.png")
  self.sp_lock_down = rl.LoadTexture("../luasrc/imgs/levels/door_lock_down.png")
  self.sp_unlock_up= rl.LoadTexture("../luasrc/imgs/levels/door_unlock_up.png")
  self.sp_unlock_down = rl.LoadTexture("../luasrc/imgs/levels/door_unlock_down.png")
  self.has_submit = false
  self.pins = {
    {'output', 1, 'unlock_door_button'},
    {'output', 1, 'lock_door_button'},
    {'input', 1, 'door_locked'},
  }
  self.schedule = {
    {unlock_door_button=0, lock_door_button=0, door_locked=nil, name='Initial state: no button pressed.'},
    {unlock_door_button=1, lock_door_button=0, door_locked=0, name='Unlock door button pressed: door should be unlocked. (door_locked=0)'},
    {unlock_door_button=0, lock_door_button=0, door_locked=0, name='Unlock door button released: door should remain unlocked. (door_locked=0)'},
    {unlock_door_button=1, lock_door_button=0, door_locked=0, name='Unlock door button pressed (again): door should be unlocked. (door_locked=0)'},
    {unlock_door_button=0, lock_door_button=0, door_locked=0, name='Unlock door button released: door should remain unlocked. (door_locked=0)'},
    {unlock_door_button=0, lock_door_button=1, door_locked=1, name='Lock door button pressed: door should be locked. (door_locked=1)'},
    {unlock_door_button=0, lock_door_button=0, door_locked=1, name='Lock door button released: door should stay open. (door_locked=1)'},
    {unlock_door_button=0, lock_door_button=1, door_locked=1, name='Lock door button pressed (again): door should be locked. (door_locked=1)'},
    {unlock_door_button=0, lock_door_button=0, door_locked=1, name='Lock door button released: door should stay open. (door_locked=1)'},
    {unlock_door_button=1, lock_door_button=0, door_locked=0, name='Unlock door button pressed: door should be unlocked. (door_locked=0)'},
    {unlock_door_button=0, lock_door_button=0, door_locked=0, name='Unlock door button released: door should remain unlocked. (door_locked=0)'},
    {unlock_door_button=0, lock_door_button=1, door_locked=1, name='Lock door button pressed: door should be locked. (door_locked=1)'},
    {unlock_door_button=0, lock_door_button=0, door_locked=1, name='Lock door button released: door should stay open. (door_locked=1)'},
  }
end

function Door:customDraw(rt)
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

  local sprites = self.sp_base

  rl.rlTranslatef(-sprites.width - 5, 0, 0)

  function _draw(s)
    local seg_width = s.width
    local seg_height = s.height
    local source = ffi.new('Rectangle', {0, 0, seg_width, seg_height})
    local target = ffi.new('Rectangle', {0, 0, source.width, source.height})
    local origin = ffi.new('Vector2', {0})
    local c = c2 -- ; ffi.new('Color', {  102, 191, 255, 255 })
    utils.rlDrawTexturePro(s, source, target, origin, 0, c);
  end

  local unlock_btn = self._buffers.output.unlock_door_button[1]
  local lock_btn = self._buffers.output.lock_door_button[1]
  local closed = self._buffers.nextIn.door_locked[1]

  _draw(self.sp_base)

  if closed== 1 then
    _draw(self.sp_closed)
  else
    _draw(self.sp_open)
  end


  if lock_btn == 1 then
    _draw(self.sp_lock_down)
  else
    _draw(self.sp_lock_up)
  end

  if unlock_btn == 1 then
    _draw(self.sp_unlock_down)
  else
    _draw(self.sp_unlock_up)
  end

  rl.rlPopMatrix()
end

addLevel({
  icon="../luasrc/imgs/levels/door_icon.png",
  name="Door Lock",
  desc=[[

!img:imgs/levels/door_img1.png

Create a simple door lock system.

The system has 2 buttons:
- one to lock the door (`lock_door_button`)
- one to unlock the door (`unlock_door_button`)

Each button is connected to a wire that goes into the circuit. When the button is not being pressed, the wire value for the button is 0, and when the button is down (ie being pressed), the wire value is 1.

When the `lock_door_button` button is pressed, the door should be locked, and should remain locked until the unlock button is pressed. Same for the unlock button.

The circuit should return wether the door is locked or not:
- `door_locked`=1  --> The door is LOCKED.
- `door_locked`=0  --> The door is UNLOCKED.

The initial value of `door_locked` is not important.

The buttons will never be pressed at the same time.

!hl

`Hint: The SR Latch`

This section will show one way to store 1 bit of memory.

It's generally not possible to store memory without using cycles, ie, wires that have outputs interlinked with some inputs somehow. We generally want to avoid arbitrarily using cycles in circuits because they can make the circuit complicated and can introduce oscillation errors, but bit storage is an exception.

The `SR latch` is a simple interlinked configuration of gates that is stable and can "remember" its previous configuration. You can change its values using the right inputs:
!img:imgs/tutorial/mem1.png
The `S` and `R` inputs stand for `set` and `reset`: you can see that when S=1 and R=0, we have the value of `Q` updated to 1, and when R=1 and S=0, the value of `Q` is "reseted" to 0. When inputs are 0,0 the latch doesnt do anything, it just maintains the previous set result, thus working as a 1-bit storage memory. The S=1 and R=1 case is a bit tricky: it makes the output be (1, 1) which is considered invalid, because we want the `Q` and `Q'` values to be opposite, so in practice we try not to use this case.

Mind that in the schema picture, we use `S'` and `R'` as inputs, they are the inverted values of `S` and `R` respectively.


    ]],
    chips = {
      Clock(true),
      Door(),
    },
    id='DOOR',
})



