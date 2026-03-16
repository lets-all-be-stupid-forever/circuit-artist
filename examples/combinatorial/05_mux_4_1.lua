require '_framework'

-- Tests for a 4:1 mux
-- This example shows that you can create the test table with code too!
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
              a=a,
              b=b,
              c=c,
              d=d,
              s=s,
              y=y,
              name='dcba=' .. d .. c .. b .. a ..  ' s=' .. s ..  ' --> y=' .. y .. '(' .. name[s+1] .. ')'
            })
          end
        end
      end
    end
  end
  return t
end

cases = makeTable()

ports = {
  {name="a", width=1, dir='in'},
  {name="b", width=1, dir='in'},
  {name="c", width=1, dir='in'},
  {name="d", width=1, dir='in'},
  {name="s", width=2, dir='in'},
  {name="y", width=1, dir='out'},
}
