Import "shared/comb_level.lua"


local abc = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
local lut = 'EKMFLGDQVZNTOWYHXUSPAIBRCJ'


function makeCases()
  local out = {}
  for i = 1, 26 do
    local n = 1 << (i - 1)
    local ch = string.sub(lut, i, i)
    local r = 1 << (string.byte(ch) - string.byte('A'))
    table.insert(out, {entry=n, sub=r, name=string.sub(abc, i, i) .. ' -> ' .. ch})
  end
  return out
end

easyAddTest({
  cases = makeCases(),
  ports = {
    {name="entry", width=26, input=false},
    {name="sub", width=26, input=true},
  }
})
