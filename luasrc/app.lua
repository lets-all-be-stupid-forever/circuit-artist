package.path = '../luasrc/?.lua;' .. package.path
package.path = '../luasrc/?/init.lua;' .. package.path
local R = require 'raylib_api'
local C = require 'c_api'
local ffi = require 'ffi'
require 'api'

initApp()
-- initPaletteFromImage("../luasrc/imgs/sample_palette3.png")
apiLoadLevel(0)
