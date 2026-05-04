Import "combfw"

ports = {
  {name="n",   dir='in',  width=8},
  {name="ans", dir='out', width=1, side='right'},
}


local mof3_cases = {
  1, 2, 4, 8, 16, 32, 64, 128,
  0, 255, 123, 45, 111, 100, 48,
  3, 33, 31, 129
}

-- Generates the multiple of 3 respoonses
cases = {}
for i=1,#mof3_cases do
  local n = mof3_cases[i]
  local ans = 0
  if (n % 3) == 0 then
    ans = 1
  end
  local name = 'n=' .. n
  table.insert(cases, {n=n, ans=ans, name=name})
end
