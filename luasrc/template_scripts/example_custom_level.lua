--[[
This is just a simple example on how to create a level.
You can check luasrc/levels for more details in the default levels.
--]]
require 'api'
local BasicROM = require 'component_examples.basic_rom'
local Clock = require 'clock'

addLevel({
  icon= '../luasrc/scripts/my_custom_icon.png',
  name= 'My Level',
  desc= [[
  My level `description`.
  ]],
  chips={
    Clock(),
    BasicROM()}
})
