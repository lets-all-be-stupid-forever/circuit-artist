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

--- Check if a file or directory exists in this path
function utils.exists(file)
  local ok, err, code = os.rename(file, file)
  if not ok then
    if code == 13 then
      -- Permission denied, but it exists
      return true
    end
  end
  return ok, err
end

--- Check if a directory exists in this path
function utils.isdir(path)
  -- "/" works on both Unix and Windows
  return exists(path.."/")
end

function utils.isModuleAvailable(name)
  if package.loaded[name] then
    return true
  else
    for _, searcher in ipairs(package.searchers or package.loaders) do
      local loader = searcher(name)
      if type(loader) == 'function' then
        package.preload[name] = loader
        return true
      end
    end
    return false
  end
end

function utils.iswindows()
   return package.config:sub(1,1) == '\\'
end


return utils
