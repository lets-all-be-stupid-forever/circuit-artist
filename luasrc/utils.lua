local math = require 'math'
local bit = require 'bit'
local ffi = require 'ffi'
local C = require 'c_api'
local utils = {}
local lshift, rshift, rol = bit.lshift, bit.rshift, bit.rol
local bnot, band, bor, bxor = bit.bnot, bit.band, bit.bor, bit.bxor

function utils.randomBits(n)
  local r = 0
  for ibit=1,n do
    local v = math.random(2)
    if v == 1 then
      r = bor(r, lshift(1, ibit-1))
    end
  end
  return r
end

function utils.randomBits32()
  local r = 0
  for ibit=1,32 do
    local v = math.random(2)
    if v == 1 then
      r = bor(r, lshift(1, ibit-1))
    end
  end
  return r
end

function utils.bget(n, ibit)
  local m = lshift(1, ibit)
  local r = rshift(band(n, m), ibit)
  return r
end

function utils.scriptPath()
  local str = debug.getinfo(2, "S").source:sub(2)
  return str:match("(.*/)")
end

function utils.rlDrawTexturePro(texture, source, dest, origin, rotation, tint)
  local args = ffi.new('RlDrawTextureProArgs', { texture, source, dest, origin, rotation, tint})
  C.RlDrawTexturePro(args)
end

return utils
