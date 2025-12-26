
--  {
--     {a=0, b=0, a_and_b=0, name='0 AND 0 = 0'},
--     {a=1, b=0, a_and_b=0, name='0 AND 1 = 0'},
--     {a=0, b=1, a_and_b=0, name='1 AND 0 = 0'},
--     {a=1, b=1, a_and_b=1, name='1 AND 1 = 1'},
--   },

function makeTable()
  local t = {}
  local name={'a', 'b', 'c', 'd'}
    for a=0,1 do
    for b=0,1 do
    for c=0,1 do
    for d=0,1 do
    for s=0,3 do
      local data = {a, b, c, d}
      local y = data[s+1]
      table.insert(t,
      {
        a=a, b=b, c=c, d=d, s=s, y=y,
        name='dcba=' .. d .. c .. b .. a ..  ' s=' .. s ..  ' --> y=' .. y .. '(' .. name[s+1] .. ')'
      }
      )
    end
    end
    end
    end
    end
    return t
end

easyAddTest({
  cases = makeTable(),
  ports = {
    {name="a", width=1, input=false},
    {name="b", width=1, input=false},
    {name="c", width=1, input=false},
    {name="d", width=1, input=false},
    {name="s", width=2, input=false},
    {name="y", width=1, input=true},
  }
})
