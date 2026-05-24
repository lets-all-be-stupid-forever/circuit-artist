Import "shared/comb_level.lua"

math.randomseed(0)

function randval(n)
  return math.random(0, n)
end

local function arshift(x, n)
  x = x & 0xFFFFFFFF  -- truncate to 32-bit
  if n >= 32 then
    return (x & 0x80000000) ~= 0 and 0xFFFFFFFF or 0
  end
  local shifted = x >> n
  if x & 0x80000000 ~= 0 then
    shifted = shifted | (~0xFFFFFFFF >> n) & 0xFFFFFFFF -- fill upper bits with 1s
    -- simpler alternative: shifted = shifted | ((0xFFFFFFFF << (32 - n)) & 0xFFFFFFFF)
  end
  return shifted
end
local function rshift(x, n)
  return x >> n
end

local function lshift(x, n)
   return (x <<n) & ((1<<32)-1)
end




function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function makeCases()
  local out = {}

  local N = 100
  for i =1,N do
    local a = randbits(32)
    local n = math.random(32) - 1
    local y = lshift(a, n)
    local name = a .. ' SLL ' .. n  .. ' = ' .. y
    table.insert(out, {
      a=a, n=n, y=y,
      sll=1, srl=0, sra=0,
      name=name,
    })
  end


  for i =1,N do
    local a = randbits(32)
    local n = math.random(32) - 1
    local y = rshift(a, n)
    local name = a .. ' SRL ' .. n  .. ' = ' .. y
    table.insert(out, {
      a=a, n=n, y=y,
      sll=0, srl=1, sra=0,
      name=name,
    })
  end

  for i =1,N do
    local a = randbits(32)
    local n = math.random(32) - 1
    local y = arshift(a, n)
    local name = a .. ' SRA ' .. n  .. ' = ' .. y
    table.insert(out, {
      a=a, n=n, y=y,
      sll=0, srl=0, sra=1,
      name=name,
    })
  end

  return out
end


easyAddTest({
  ports = {
    {name="a", width=32, input=false},
    {name="n", width=5, input=false},
    {name="sll", width=1, input=false},
    {name="srl", width=1, input=false},
    {name="sra", width=1, input=false},
    {name="y", width=32, input=true, fmt=FormatBinary},
  },
  cases = makeCases()
})
