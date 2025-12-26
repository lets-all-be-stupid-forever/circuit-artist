math.randomseed(0)

function randbits(n)
  return math.random(0, math.tointeger((1<<n) - 1))
end

function makeCases()
  local tab = {}
  for i=1,20 do
    for s=0,3 do
      for t=0,3 do
        local c = {
          randbits(2),
          randbits(2),
          randbits(2),
          randbits(2),
        }
        local v = {0,0,0,0}
        v[t+1] = c[s+1]
        table.insert(tab, {
          origin=s,
          destination=t,
          C0_out=c[1],
          C1_out=c[2],
          C2_out=c[3],
          C3_out=c[4],
          C0_in=v[1],
          C1_in=v[2],
          C2_in=v[3],
          C3_in=v[4],
          name='Sending C' .. s .. ' to C' .. t
        })
      end
    end
  end
  return tab
end

easyAddTest({
  ports = {
    {name="origin", width=2, input=false},
    {name="destination", width=2, input=false},
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



