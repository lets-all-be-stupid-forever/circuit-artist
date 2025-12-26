math.randomseed(0)

function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function makeCases(nbits)
  local cases = {}

  local function addcase(a, b)
    local r = 0
    if (a==b) then r = 1 end
    print(a, math.type(a))
    print(b, math.type(b))
    -- TODO put an assert error here and debug
    assert (math.type(a) == 'integer')
    assert (math.type(b) == 'integer')
    table.insert(cases, {
      a=a,
      b=b,
      a_eq_b=r,
      name='A eq B = ' .. r,
    })
  end

  local m = (math.tointeger(1) << nbits) - 1;
  for i=1,nbits do
    local n = math.tointeger(randbits(nbits))
    local f = math.tointeger((1 << (i - 1)))
    local a = n ~ f
    local b = ((~n) & m)
    addcase(a, a)
    addcase(a, 0)
    addcase(a, m)
    addcase(a, b)
  end
  for i=1,100 do
    local n = randbits(nbits)
    local m = randbits(nbits)
    addcase(n, n)
    addcase(m, m)
    addcase(m, n)
  end
  return cases
end

local nbits = 32
easyAddTest({
  cases = makeCases(nbits),
  ports = {
    {name="a", width=nbits, input=false},
    {name="b", width=nbits, input=false},
    {name="a_eq_b", width=1, input=true},
  }
})
