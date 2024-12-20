package.path = '../luasrc/?.lua;' .. package.path
package.path = '../luasrc/?/init.lua;' .. package.path
local R = require 'raylib_api'
local C = require 'c_api'
local ffi = require 'ffi'
local utils = require 'utils'
require 'api'

initTutorial()

-- Loads each level
require 'levels.sandbox'
require 'levels.seven_seg'
require 'levels.bus'
require 'levels.mof3'
require 'levels.simple_ram'
require 'levels.custom_components_example'
require 'levels.collatz'
require 'levels.gcd'
require 'levels.riscv_alu'
require 'levels.hanoi'
-- require 'levels.primes'

loadProgress()

setInitialLevelByName('Sandbox')

if utils.isModuleAvailable('scripts') then
  require('scripts')
  C.CaAddMessage("Imported custom scripts.", 5)
else
  print('scripts not found')
end

apiLoadLevel()
