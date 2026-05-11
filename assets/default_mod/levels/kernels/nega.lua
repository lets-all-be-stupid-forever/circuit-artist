Import "shared/comb_level.lua"

math.randomseed(0)

function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function fixsign(n)
  if (n & 128) ~= 0 then
    return (n-256)
  else
    return n
  end
end

function makeCases()
  local m = 100;
  local out = {}
  for i = 1,m do
    local n = randbits(8)
    local r = (~n + 1) & 255
    local nn = fixsign(n)
    local rr = fixsign(r)
    table.insert(out, {A=n, neg_A=r, name='-(' .. nn .. ')=' .. rr})
  end
  return out
end



--[[
--old code for abs(A)
function makeCases()
  local m = 100;
  local out = {}
  for i = 1,m do
    local n = randbits(8)
    local r = n
    local m = n
    if (r & 128) ~= 0 then
      r = (~r + 1) & 255
      m = (n-256) -- 255=-1, 254=-2,  128 = -128
    end
    table.insert(out, {A=n, abs_A=r, name='abs(' .. m .. ')=' .. r})
  end
  return out
end
]]


easyAddTest({
  ports = {
    {name="A", width=8, input=false},
    {name="neg_A", width=8, input=true},
  },
  cases = makeCases(),
})
