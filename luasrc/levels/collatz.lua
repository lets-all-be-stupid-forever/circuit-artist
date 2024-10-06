local Tester = require 'tester'
local Clock = require 'clock'
local Collatz = Tester:extend()

function Collatz:new()
  Collatz.super.new(self)
  self.pins = {
    {'input', 1, 'submit'},
    {'output', 8, 'n'},
    {'input', 8, 'steps'},
  }
  self.has_submit = true
  self.schedule = {
    {n=1,steps=0  },
    {n=2,steps=1  },
    {n=4,steps=2  },
    {n=5,steps=5  },
    {n=3,steps=7  },
    {n=6,steps=8  },
    {n=7,steps=16 },
    {n=8,steps=3  },
    {n=9,steps=19 },
    {n=10  ,steps=6  },
    {n=11  ,steps=14 },
    {n=12  ,steps=9  },
    {n=13  ,steps=9  },
    {n=14  ,steps=17 },
    {n=15  ,steps=17 },
    {n=16  ,steps=4  },
    {n=27  ,steps=111 },
    {n=41  ,steps=109 },
    {n=100  ,steps=25  },
    {n=123  ,steps=46  },
--   {n=333  ,steps=112  }, These ones need more bits
--    {n=512  ,steps=9  },
--    {n=999  ,steps=49  },
--    {n=703  ,steps=170  },
  }

  for i=1,#self.schedule do
    local s = self.schedule[i]
    s.name = 'n=' .. s.n .. ' (steps should be ' .. s.steps .. ')'
  end

end

addLevel({
  icon = "../luasrc/imgs/levels/collatz_icon.png",
  name = "3x+1 Sequence",
  desc=[[

The 3x+1 sequence is defined by:
* x = x/2 if x is even
* x = 3x+1 if x is odd

Find the number of steps needed for the sequence starting with `n` to reach 1.

!img:imgs/levels/collatz_img1.png

You can use `multiple clock cycles` to compute the result. The result will only be verified when the `submit` flag is on. The check is done on the rising edge of the clock. After verification is passed, the next test case will be provided.

`Examples:`
n=1 --> steps=0
n=2 --> steps=1 (2 -> 1)
n=3 --> steps=7 (3 -> 10 -> 5 -> 16 -> 8 -> 4 -> 2 -> 1)
n=4 --> steps=2 (4 -> 2 -> 1)

Inputs:
- `n` (8bits): positive number corresponding to the initial value of the sequence.

Outputs:
- `submit` (1bit): flag saying when the computation is done and result is ready
- `steps` (8bits): Result of calculation, containing the number of steps for the sequence starting at `n` to reach the number 1.

The first 16 tests will be the positive numbers from `n`=1 to `n`=16. The 8 next numbers will be other positive numbers between 16 and 128. The sequence size is guaranteed to be shorter than 255.

]],
  chips = {
    Clock(),
    Collatz(),
  },
  id='COLLATZ',
  unlockedBy='RAM8',
})
