Import "shared/comb_level.lua"

math.randomseed(0)

function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function makeCases()
  local m = 100;
  local out = {}
  for i = 1,m do
    local n = randbits(8)
    -- 5 = 101
    local r = math.tointeger(n*3)
    table.insert(out, {A=n, A_mul_3=r, name='' .. n .. 'x3=' .. r})
  end
  return out
end


easyAddTest({
  ports = {
    {name="A", width=8, input=false},
    {name="A_mul_3", width=10, input=true},
  },
  cases = makeCases(),
})
