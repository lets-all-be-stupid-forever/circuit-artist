Import "shared/comb_level.lua"

math.randomseed(0)

function randval(n)
  return math.random(0, n)
end


function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function makeCases()
  local out = {}
  for i=1,300 do
    local n = randval(4)
    local a = randbits(4)
    local b = randbits(4)
    local r = ((((a << 4) + b) >> n) & 15)

    table.insert(out, {
      a=a, b=b, n=n, r=r,
      name='funnel('.. a ..'++' .. b ..',' .. n .. ') = ' .. r,
    })
  end
  return out
end

easyAddTest({
  ports = {
    {name="b", width=4, input=false},
    {name="a", width=4, input=false},
    {name="n", width=3, input=false},
    {name="r", width=4, input=true, fmt=FormatBinary},
  },
  cases = makeCases(),
})
