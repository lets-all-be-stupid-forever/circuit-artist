local Tester = require 'tester'
local Clock = require 'clock'
local rl = require 'raylib_api'
local ffi = require 'ffi'
local utils = require 'utils'
local Camera = Tester:extend()

function Camera:new()
  Camera.super.new(self)
  -- self.sp_base = rl.LoadTexture("../luasrc/imgs/levels/door_base.png")
  -- self.sp_open = rl.LoadTexture("../luasrc/imgs/levels/door_open.png")
  -- self.sp_closed = rl.LoadTexture("../luasrc/imgs/levels/door_closed.png")
  -- self.sp_lock_up= rl.LoadTexture("../luasrc/imgs/levels/door_lock_up.png")
  -- self.sp_lock_down = rl.LoadTexture("../luasrc/imgs/levels/door_lock_down.png")
  -- self.sp_unlock_up= rl.LoadTexture("../luasrc/imgs/levels/door_unlock_up.png")
  -- self.sp_unlock_down = rl.LoadTexture("../luasrc/imgs/levels/door_unlock_down.png")
  self.has_submit = false
  self.pins = {
    {'output', 8, 'sensor'},
    {'output', 1, 'take_photo_button_pressed'},
    {'input', 8, 'photo'},
  }

  -- Random sequence of numbers
  local seq={170,236,142,222,141,92,52,241,18,132,65,186,240,15,115,64,181,45,95,39,161,10,
  157,149,252,24,81,54,77,210,5,231,49,93,34,246,22,150,84,219,228,97,154,9,165,
  176,205,235,37,88,134,226,155,86,232,113,13,7,223,172,138,40,99,123,218,151,230,
  129,220,244,125,190,28,96,121,247,106,168,118,227,249,215,208,139,242,237,42,
  203,253,43,200,91,184,30,211,29,140,85,201,87,130,146,238,212,225,26,73,53,144,
  221,175,133,68,191,78,79,75,31,254,147,103,8,189,44,214,213,120,50,105,117,207,
  2,193,206,102,239,229,164,152,192,59,156,23,27,255,143,16,90,148,251,217,111,
  187,216,80,110,209,33,197,66,41,173,14,61,182,63,204,11,74,62,36,178,98,12,83,
  94,177,32,126,89,245,57,21,250,198,159,35,202,6,162,48,56,127,47,67,174,224,166,
  72,136,71,188,234,160,183,82,108,38,76,180,124,167,233,20,51,171,109,131,196,
  116,199,19,145,137,4,179,55,114,46,135,185,100,104,248,101,243,70,3,119,112,195,
  163,17,122,1,107,128,58,153,25,60,69};


  -- 0 --> changes sensor
  -- 1 --> butotn down
  -- 2 --> button up
  -- 3 --> button down and sensor changes at same time.
  local plan = {0, 0, 0, 0, 1, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2,1,2,1,2,0,1,3,0,1,3,1,3,0,0,0};

  local iseq = 1;
  local photo = nil;
  local btn = 0;
  local sensor = nil;
  local schedule = {}
  for j=1,2 do
  for i=1,#plan do
    local act = plan[i];
    local name = nil;
    if act == 0 then
      sensor = seq[iseq];
      name = 'Sensor changed to ' .. sensor .. '.'
      iseq = iseq + 1;
    elseif act == 1 then
      name='Photo button down! Photo of should be taken.'
      btn = 1;
      photo = sensor;
    elseif act == 2 then
      name='Photo button released.'
      btn=0;
    elseif act == 3 then
      name='Photo button released and sensor changed.'
      sensor = seq[iseq]
      iseq = iseq + 1
      btn=0
    end
    table.insert(schedule, {
      sensor=sensor,
      photo=photo,
      take_photo_button_pressed=btn,
      name=name,
    })
  end
  end

  self.schedule = schedule;
end

addLevel({
  icon="../luasrc/imgs/levels/camera_icon.png",
  name="Camera",
  desc=[[

!img:imgs/levels/camera_img1.png

Build a simple camera that takes photos of 8-bit numbers.

The camera has:
- A `sensor` that receives light non-stop, and send the light values to the sensor 8-bit input to the circuit.
- A "take photo" `button`, that can have pressed (button_pressed=1) and non-pressed (button_pressed=0) states.
- An 8-bit output `display`.

Whenever the user presses the photo button, the output display should show the number present in the display at that EXACT moment. When the user releases the photo button, the display should still show the last taken photo. When the user presses the button again, the photo should again change.

The sensor values may change while the user is still pressing the photo button, and even if it does, the output photo should not change: the output should extract the value of the sensor at the EXACT moment where the button is pressed.

At the exact moment the user presses the photo button, the sensor will not change.

!img:imgs/levels/camera_img2.png

!hl

`Hint: You just need to solve for 1 bit sensor, then repeat x8.`
`Hint: The D Flip Flop`

!img:imgs/tutorial/mem8.png

Sometimes you want the memory to be updated only once when the E is set to 1.

Then, you can perform your calculations however you want, have the `D` bit modified with the new (next) value of the storage without interfering the current storage/calculation. Then, the storage is only updated again whenever the E goes to 0 and then back again to 1 ! (ie, in the next `CYCLE`, creating a proper sequential mechanism)

!img:imgs/tutorial/mem9.png

This can be achieved with a `D flip flop`, that can be created a using D latches.

!img:imgs/tutorial/mem10.png

We call this behaviour a `rising edge-enabled` memory, in contrast with the previous `level-enabled` memory of `D Latches`.  Below a comparison between the two storage modes:
!img:imgs/tutorial/mem13.png

    ]],
    chips = {
      Clock(true),
      Camera(),
    },
    id='CAMERA',
})





