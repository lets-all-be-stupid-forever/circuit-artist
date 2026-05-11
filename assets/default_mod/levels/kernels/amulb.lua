Import "shared/comb_level.lua"

math.randomseed(0)

function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function makeCases()
  local out = {}

  local function add(a,b)
    local r = math.tointeger(a*b)
    table.insert(out, {A=a, B=b, A_mul_B=r, name='' .. a .. 'x' .. b .. '=' .. r})
  end
  add(0,0)
  add(1,0)
  add(0,1)
  add(1,1)
  add(255,1)
  add(1,15)
  add(2,15)
  add(255,15)
  add(255,2)
  for i = 1,200 do
    local a = randbits(8)
    local b = randbits(4)
    add(a, b)
  end
  return out
end


easyAddTest({
  ports = {
    {name="A", width=8, input=false},
    {name="B", width=4, input=false},
    {name="A_mul_B", width=12, input=true},
  },
  cases = makeCases(),
})

