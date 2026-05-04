Import "submitfw"

local function gcd(a,b)
  if type(a) == "number" and type(b) == "number" and
    a == math.floor(a) and b == math.floor(b) then
    if b == 0 then
      return a
    else
      return gcd(b, a % b) -- tail recursion
    end
  else
    error("Invalid argument to gcd (" .. tostring(a) .. "," ..
    tostring(b) .. ")", 2)
  end
end

ports = {
  {dir='in', width=16, name='a'},
  {dir='in', width=16, name='b'},
  {dir='out', width=16, name='gcd_ab'},
}

math.randomseed(0)

cases = {}

local k = math.floor(32000 / (24*24))
for i=1,24 do
  local a = math.random(k * i * i) + 1
  local b = math.random(k * i * i) + 1
  local gcd_ab = gcd(a,b)
  cases[i] = {
    a=a,
    b=b,
    gcd_ab=gcd_ab,
    name='GCD(a=' .. a .. ', b=' .. b .. ') should be ' .. gcd_ab,
  }
end
