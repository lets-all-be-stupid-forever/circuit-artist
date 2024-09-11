local Tester = require 'tester'
local Clock = require 'clock'

local Primes = Tester:extend()

function calcPrimes(n)
  -- p[i] = true if i is prime, false otherwise
  local p = {}
  for i=2,n do
    p[i] = true
  end
  for i=2,n do
    if p[i] then
      for j=2,n do
        local m = j*i
        if m > n then
          break
        end
        p[m] = false
      end
    end
  end
  return p
end

function Primes:new(n)
  Primes.super.new(self)
  self.has_submit = true
  self.pins = {
    {'input', 1, 'submit'},
    {'output', 8, 'n'},
    {'input', 1, 'isprime'},
  }
  local primes = calcPrimes(n)
  local schedule = {}
  for i = 2,n do
    local r = 0
    if primes[i] then r = 1 end
    local extra = '(not prime)'
    if r == 1 then extra = '(prime)' end
    table.insert(schedule, {n=i, isprime=r, name='n=' .. i .. ' ' .. extra})
  end
  self.schedule = schedule
end

return {
    icon = "../luasrc/imgs/levels/primes_icon.png",
    name = "Primes",
    desc=[[

!img:imgs/levels/primes_img1.png

Find if an 8-bit (unsigned) positive number `n` > 1 is a prime.

You can use `multiple clock cycles` to compute the result. The result will only be verified when the `submit` flag is on. The check is done on the rising edge of the clock. After verification is passed, the next test case will be provided.

    ]],
    chips = {
      Clock(),
      Primes(250),
    }
}
