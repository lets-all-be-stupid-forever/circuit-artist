function makeTable()
  local t = {}
  local name={'a', 'b'}
    for a=0,3 do
    for b=0,3 do
    for s=0,1 do
      local data = {a, b}
      local y = data[s+1]
      table.insert(t,
      {
        a=a, b=b, s=s, y=y,
        name='a=' .. a .. ' b=' .. b .. ' s=' .. s ..  ' --> y=' .. y .. '(' .. name[s+1] .. ')'
      }
      )
    end
    end
    end
    return t
end

easyAddTest({
  cases = makeTable(),
  ports = {
    {name="a", width=2, input=false},
    {name="b", width=2, input=false},
    {name="s", width=1, input=false},
    {name="y", width=2, input=true},
  }
})
