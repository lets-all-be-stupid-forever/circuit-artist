package.path = '../luasrc/?.lua;' .. package.path
package.path = '../luasrc/?/init.lua;' .. package.path
local R = require 'raylib_api'
local C = require 'c_api'
local ffi = require 'ffi'
local utils = require 'utils'
require 'api'

initTutorial()

-- Loads each level
addLevel(require 'levels.sandbox')
addLevel(require 'levels.seven_seg')
addLevel(require 'levels.bus')
addLevel(require 'levels.mof3')
addLevel(require 'levels.simple_ram')
addLevel(require 'levels.collatz')
addLevel(require 'levels.gcd')
addLevel(require 'levels.riscv_alu')
addLevel(require 'levels.hanoi')
addLevel(require 'levels.primes')

setInitialLevelByName('Sandbox')

if utils.isModuleAvailable('scripts') then
  require('scripts')
  C.CaAddMessage("Imported custom scripts.", 5)
else
  print('scripts not found')
end

apiLoadLevel()
