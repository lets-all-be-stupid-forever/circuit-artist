Import "shared/comb_level.lua"

comb_level[[
conn out 4 a
conn in 4 shifted_a
test  1000 1100 a=-8 -> shifted_a=-4
test  1001 1100 a=-7 -> shifted_a=-4
test  1010 1101 a=-6 -> shifted_a=-3
test  1011 1101 a=-5 -> shifted_a=-3
test  1100 1110 a=-4 -> shifted_a=-2
test  1101 1110 a=-3 -> shifted_a=-2
test  1110 1111 a=-2 -> shifted_a=-1
test  1111 1111 a=-1 -> shifted_a=-1
test  0000 0000 a=0 -> shifted_a=0
test  0001 0000 a=1 -> shifted_a=0
test  0010 0001 a=2 -> shifted_a=1
test  0011 0001 a=3 -> shifted_a=1
test  0100 0010 a=4 -> shifted_a=2
test  0101 0010 a=5 -> shifted_a=2
test  0110 0011 a=6 -> shifted_a=3
test  0111 0011 a=7 -> shifted_a=3
]]
