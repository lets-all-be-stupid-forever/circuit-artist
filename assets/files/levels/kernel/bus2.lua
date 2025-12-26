math.randomseed(0)

function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function makeCases()

  local t = {}
  for i=1,20 do
    local c = {
      randbits(2),
      randbits(2),
      randbits(2),
      randbits(2),
    }
    for s=0,3 do
      local v = c[s+1]
      table.insert(t, {
        selector=s,
        C0_out=c[1],
        C1_out=c[2],
        C2_out=c[3],
        C3_out=c[4],
        C0_in=v,
        C1_in=v,
        C2_in=v,
        C3_in=v,
        name='Sending C' .. s
      })

    end
  end
  return t
end

easyAddTest({
  ports = {
    {name="selector", width=2, input=false},
    {name="C0_in", width=2, input=true},
    {name="C0_out", width=2, input=false},
    {name="C1_in", width=2, input=true},
    {name="C1_out", width=2, input=false},
    {name="C2_in", width=2, input=true},
    {name="C2_out", width=2, input=false},
    {name="C3_in", width=2, input=true},
    {name="C3_out", width=2, input=false},
  },
  cases = makeCases(),
})

