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
require 'levels.bus'
require 'levels.seven_seg'
require 'levels.mof3'
require 'levels.simple_ram'
require 'levels.collatz'
require 'levels.gcd'
require 'levels.riscv_alu'
require 'levels.hanoi'
require 'levels.custom_components'
-- require 'levels.primes'


-- Legacy levels
-- require 'levels.legacy_levels.wires'
-- require 'levels.legacy_levels.nand'
-- require 'levels.legacy_levels.not'
-- require 'levels.legacy_levels.and'
-- require 'levels.legacy_levels.or'
-- require 'levels.legacy_levels.xor'
-- require 'levels.legacy_levels.mux'
-- require 'levels.legacy_levels.decoder'
-- require 'levels.legacy_levels.demux'
-- require 'levels.legacy_levels.adder'
-- require 'levels.legacy_levels.subtractor'
-- require 'levels.legacy_levels.comparator'
-- require 'levels.legacy_levels.shifter'

loadProgress()

setInitialLevelByName('Sandbox')

if utils.isModuleAvailable('scripts') then
  require('scripts')
  C.CaAddMessage("Imported custom scripts.", 5)
else
  print('scripts not found')
end

apiLoadLevel()
