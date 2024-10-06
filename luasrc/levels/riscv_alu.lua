local Tester = require 'tester'
local Clock = require 'clock'
local RiscvALU= Tester:extend()
local math = require 'math'
local utils = require 'utils'

local bit = require 'bit'
local bnot = bit.bnot
local band, bor, bxor = bit.band, bit.bor, bit.bxor
local lshift, rshift, arshift, rol = bit.lshift, bit.rshift, bit.arshift, bit.rol

function RiscvALU:new()
  self.super.new(self)
  self.pins = {
    {'output', 32, 'A'},
    {'output', 32, 'B'}, -- 100 -> 1byte 010 -> 2bytes 001 --> 4 bytes
    {'input', 32, 'Y'},
    {'input', 1, 'GT'},
    {'input', 1, 'EQ'},
    {'input', 1, 'LT'},
    {'output', 1, 'ADD'},
    {'output', 1, 'SUB'},
    {'output', 1, 'XOR'},
    {'output', 1, 'OR'},
    {'output', 1, 'AND'},
    {'output', 1, 'SLL'},
    {'output', 1, 'SRL'},
    {'output', 1, 'SRA'},
    {'output', 1, 'SLT'},
    {'output', 1, 'SLTU'},
  }

  function makebase()
    return {
      A=0,
      B=0,
      Y=nil,
      GT=nil,
      EQ=nil,
      LT=nil,
      ADD=0,
      SUB=0,
      XOR=0,
      OR=0,
      AND=0,
      SLL=0,
      SRL=0,
      SRA=0,
      SLT=0,
      SLTU=0,
      name='Initial'
    }
  end

  local cases = {
    makebase(),
  }

  math.randomseed(1)
  local N = 10
  local c = makebase()
  c.A = 1
  c.B = 2
  c.Y = 3
  c.ADD = 1
  c.name =  c.A .. ' + ' ..  c.B .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = 4444
  c.B = 8123
  c.Y = c.A + c.B
  c.ADD = 1
  c.name =  c.A .. ' + ' ..  c.B .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = 56
  c.B = -55
  c.Y = c.A + c.B
  c.ADD = 1
  c.name = c.A .. ' + (' ..  c.B .. ') = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = 4
  c.B = 3
  c.Y = c.A - c.B
  c.SUB = 1
  c.name = c.A .. ' - (' ..  c.B .. ') = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = 2
  c.B = 3
  c.Y = c.A - c.B
  c.SUB = 1
  c.name = c.A .. ' - (' ..  c.B .. ') = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = 8
  c.B = -8
  c.Y = c.A - c.B
  c.SUB = 1
  c.name = c.A .. ' - (' ..  c.B .. ') = ' .. c.Y
  table.insert(cases, c)


  c = makebase()
  c.A = utils.randomBits32()
  c.B = utils.randomBits32()
  c.Y = band(c.A, c.B)
  c.AND = 1
  c.name = c.A  .. ' AND ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = utils.randomBits32()
  c.B = utils.randomBits32()
  c.Y = bor(c.A, c.B)
  c.OR = 1
  c.name = c.A  .. ' OR ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  for i =1,N do
    c = makebase()
    c.A = utils.randomBits32()
    c.B = utils.randomBits32()
    c.Y = bxor(c.A, c.B)
    c.XOR = 1
    c.name = c.A  .. ' XOR ' .. c.B  .. ' = ' .. c.Y
    table.insert(cases, c)
  end

  for i =1,N do
    c = makebase()
    c.A = utils.randomBits32()
    c.B = math.random(32) - 1
    c.Y = lshift(c.A, c.B)
    c.SLL = 1
    c.name = c.A  .. ' SLL ' .. c.B  .. ' = ' .. c.Y
    table.insert(cases, c)
  end


  for i =1,N do
    c = makebase()
    c.A = utils.randomBits32()
    c.B = math.random(32) - 1
    c.Y = rshift(c.A, c.B)
    c.SRL = 1
    c.name = c.A  .. ' SRL ' .. c.B  .. ' = ' .. c.Y
    table.insert(cases, c)
  end

  for i =1,N do
    c = makebase()
    c.A = utils.randomBits32()
    c.B = math.random(32) -1
    c.Y = arshift(c.A, c.B)
    c.SRA = 1
    c.name = c.A  .. ' SRA ' .. c.B  .. ' = ' .. c.Y
    table.insert(cases, c)
  end

  c = makebase()
  c.A = 32
  c.B = 31
  c.Y = 0
  c.SLT = 1
  c.GT = 1
  c.EQ = 0
  c.LT = 0
  c.name = c.A  .. ' SLT ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = -32
  c.B = 31
  c.Y = 1
  c.GT = 0
  c.EQ = 0
  c.LT = 1
  c.SLT = 1
  c.name = c.A  .. ' SLT ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = -23
  c.B = -23
  c.Y = 0
  c.GT = 0
  c.EQ = 1
  c.LT = 0
  c.SLT = 1
  c.name = c.A  .. ' SLT ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = 32
  c.B = 31
  c.Y = 0
  c.SLTU = 1

  c.name = c.A  .. ' SLTU ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = -32
  c.B = 31
  c.Y = 0
  c.SLTU = 1
  c.name = c.A  .. ' SLTU ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  c = makebase()
  c.A = 4
  c.B = 22
  c.Y = 1
  c.SLTU = 1
  c.name = c.A  .. ' SLTU ' .. c.B  .. ' = ' .. c.Y
  table.insert(cases, c)

  self.schedule = cases

end

addLevel({
    icon = "../luasrc/imgs/levels/riscv_alu_icon.png",
    name = "RISCV ALU: RV32I",
    desc=[[


!img:imgs/levels/alu.png

Implement a 32-bit ALU (Arithmetic Logic Unit).

Note that there's no clock in here, so there's no need for memory.

The ALU works in the following way. It has some inputs and some outputs, and a set of flags saying which operation to be computed with these inputs. For example, with the addition flag you want to sum the two inputs, and so on.

For inputs, you have 2 32-bit values `A` and `B`. They're normally signed 32-bit numbers, but the interpretation may change for some operations. For example, for logic AND operation, you just treat them as 32-bit values.

There's one main 32-bit output value `Y`, which contains the result of the operation, and 3 other auxiliar outputs used to compare the two inputs, which don't depend on the operation:

- `GT` (greater than) active when `A`>`B`
- `EQ` (equals) active when `A`=`B`
- `LT` (less than) active when `A`<`B`

There's one bit flag for each operation, listed below. No more than one flag will be active at the same time.

Arithmetic operations:
- `ADD`: `Y`=`A`+`B`
- `SUB`: `Y`=`A`-`B`

Logic operations:
- `XOR`: `Y`=`A` XOR `B`
- `OR`: `Y`=`A` OR `B`
- `AND`: `Y`=`A` AND `B`

Shift operations:
- `SLL`: `Y`=`A`<<`B` (unsigned)
- `SRL`: `Y`=`A`>>`B` (unsigned)
- `SRA`: `Y`=`A`>>`B` (signed)

Comparison operations:
- `SLT`: `Y`=1 if `A`<`B` (signed), otherwise 0
- `SLTU`: `Y`=1 if `A`<`B` (unsigned), otherwise 0

Detailed explanation of each operation:

`ADD` (Addition) and `SUB` (Subtraction) are just 32-bit arithmetic on signed 2-complements binary numbers.

`AND` (Logic AND), `OR` (Logic OR) and `XOR` (Logic XOR) are 1-bit operations to be applied to each bit of `A` and `B` independently. Example (4bits): AND(1010, 0111) = 0010.

`SLL` (Logic Shift Left) is a simple shift left of the bits of `A` by `B` bits. `B` will always be a positive number between 0 and 31. In left shifts, the number is often increased. 6-bit example: SLL(000110, 1) = 001100.

`SRL` (Logic Shift Right) is the same as `SLL` but you shift the bits to the right, making the number smaller in general. This operation is useful for dividing numbers by powers of two.

`SRA` (Arithmetic Shift Right) is similar to SRL, but you need to consider the number as a signed number, so the leftmost bit sign must be kept, and whenever the number is negative you want to fill the new places with 1 instead of zeros. This operation is useful for dividing negative numbers by powers of 2. 4-bit examples :

SRA(0010, 1) = 0001 (2 --> 1)
SRA(1100, 1) = 1110 (-4 --> -2)

!img:imgs/levels/riscv_alu_img1.png

`SLT` (Set Less Than) is a signed number comparison operation, where we set the output to 1 only if `A` is smaller than `B`. Example:

SLT(3, 5)=1
SLT(-100, 3)=1
SLT(100, 3)=0

`SLTU` (Set Less Than Unsigned) is the same as SLT but the bits are interpreted as unsigned numbers instead of signed numbers.

]],
    chips = {
      Clock(true),
      RiscvALU(),
    },
    id='ALU32',
    unlockedBy='GCD',
})
