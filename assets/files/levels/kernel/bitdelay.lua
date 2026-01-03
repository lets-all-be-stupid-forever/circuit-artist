local seq={0,1,0,0,1,0,1,1,1,0,1,0,0,0,0,1,0,0,0,1,1,0,1,0,1,1,1,0,1,1,1,1,1,0,1,1,0,1,1,0,0,0}


local function makeCases()
  local r = {}
  table.insert(r, {CLK=0, A=seq[1], delayed_A=nil})
  table.insert(r, {CLK=1, A=seq[1], delayed_A=nil})
  table.insert(r, {CLK=0, A=seq[2], delayed_A=nil})
  table.insert(r, {CLK=1, A=seq[2], delayed_A=seq[1]})
  table.insert(r, {CLK=0, A=seq[3], delayed_A=seq[1]})

  for i=3,#seq-1 do
    table.insert(r, {CLK=1, A=seq[i], delayed_A=seq[i-1]})
    table.insert(r, {CLK=0, A=seq[i+1], delayed_A=seq[i-1]})
  end

  return r
end


easyAddTest({
  cases = makeCases(),
  ports = {
    {name="CLK", width=1, input=false},
    {name="A", width=1, input=false},
    {name="delayed_A", width=1, input=true},
  }
})


