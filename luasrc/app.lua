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
require 'levels.wires'
require 'levels.nand'
require 'levels.not'
require 'levels.and'
require 'levels.or'
require 'levels.xor'
require 'levels.mux'
require 'levels.decoder'
require 'levels.demux'
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

loadProgress()

setInitialLevelByName('Sandbox')

if utils.isModuleAvailable('scripts') then
  require('scripts')
  C.CaAddMessage("Imported custom scripts.", 5)
else
  print('scripts not found')
end

apiLoadLevel()
