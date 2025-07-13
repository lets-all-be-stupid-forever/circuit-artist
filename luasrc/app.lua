package.path = '../luasrc/?.lua;' .. package.path
package.path = '../luasrc/?/init.lua;' .. package.path
local R = require 'raylib_api'
local C = require 'c_api'
local ffi = require 'ffi'
local utils = require 'utils'
require 'api'

initTutorial()

-- Legacy levels
require 'legacy_levels.wires'
require 'legacy_levels.nand'
require 'legacy_levels.not'
require 'legacy_levels.and'
require 'legacy_levels.or'
require 'legacy_levels.xor'
require 'legacy_levels.mux'
require 'levels.bus'
require 'legacy_levels.decoder'
require 'legacy_levels.demux'
require 'levels.seven_seg'

require 'levels.sandbox'

require 'legacy_levels.shifter'
require 'levels.mof3'

require 'legacy_levels.adder'
require 'legacy_levels.subtractor'
require 'legacy_levels.comparator'

-- These need memory
require 'levels.collatz'
require 'levels.gcd'

-- Loads each level
require 'levels.simple_ram'
require 'levels.riscv_alu'
require 'levels.hanoi'

require 'levels.custom_components'
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
