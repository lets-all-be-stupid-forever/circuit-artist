math.randomseed(0)

function dump(o)
  if type(o) == 'table' then
    local s = '{ '
    for k,v in pairs(o) do
      if type(k) ~= 'number' then k = '"'..k..'"' end
      s = s .. '['..k..'] = ' .. dump(v) .. ','
    end
    return s .. '} '
  else
    return tostring(o)
  end
end

function randint(m)
  return math.random(0, m-1)
end


local function makeCases()
  -- first resets
  local cases = {
    {clk=0, rst=1, data=0, waddr=0, write_enable=0, y0=nil, y1=nil, y3=nil, y4=nil, name="Power On Reset"},
    {clk=1, rst=1, data=0, waddr=0, write_enable=0, y0=0, y1=0, y3=0, y4=0, name="Power On Reset"},
  }

  local nc = 200
  for i=1,nc do
    local addr = randint(4)
    local we = randint(4)
    if we ~= 0 then we = 1 else we = 0 end
    local data = randint(2)
    local prv = cases[#cases]
    local c0 = {} -- clock 0
    local c1 = {} -- clock 1

    local name = 'no op'
    if we == 1 then
      if addr == 0 then name = 'y0 <-- ' .. data
      elseif addr == 1 then name = 'y1 <-- ' .. data
      elseif addr == 2 then name = 'y2 <-- ' .. data
      elseif addr == 3 then name = 'y3 <-- ' .. data
      else
        error('Invalid address')
      end
    end

    c0['y0'] = prv['y0']
    c0['y1'] = prv['y1']
    c0['y2'] = prv['y2']
    c0['y3'] = prv['y3']
    c0['write_enable'] = we
    c0['data'] = data
    c0['rst'] = 0
    c0['clk'] = 0
    c0['waddr'] = addr
    c0['name'] = name

    c1['y0'] = prv['y0']
    c1['y1'] = prv['y1']
    c1['y2'] = prv['y2']
    c1['y3'] = prv['y3']
    c1['write_enable'] = we
    c1['data'] = data
    c1['rst'] = 0
    c1['clk'] = 1
    c1['waddr'] = addr
    c1['name'] =  name
    if we == 1 then
      if addr == 0 then c1['y0'] = data end
      if addr == 1 then c1['y1'] = data end
      if addr == 2 then c1['y2'] = data end
      if addr == 3 then c1['y3'] = data end
    end
    table.insert(cases, c0)
    table.insert(cases, c1)
  end

  return cases
end




easyAddTest({
  cases = makeCases(),
  ports = {
    {name="rst", width=1, input=false},
    {name="clk", width=1, input=false},
    {name="data", width=1, input=false},
    {name="waddr", width=2, input=false},
    {name="write_enable", width=1, input=false},
    {name="y0", width=1, input=true},
    {name="y1", width=1, input=true},
    {name="y2", width=1, input=true},
    {name="y3", width=1, input=true},
  }
})


