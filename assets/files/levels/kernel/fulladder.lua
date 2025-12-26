comb_level[[
conn out 1 cin
conn out 1 a
conn out 1 b
conn in 1 sum_a_b
conn in 1 carry_out
test 0 0 0 0 0 cin=0 a=0 b=0
test 0 1 0 1 0 cin=0 a=1 b=0
test 0 0 1 1 0 cin=0 a=0 b=1
test 0 1 1 0 1 cin=0 a=1 b=1
test 1 0 0 1 0 cin=1 a=0 b=0
test 1 1 0 0 1 cin=1 a=1 b=0
test 1 0 1 0 1 cin=1 a=0 b=1
test 1 1 1 1 1 cin=1 a=1 b=1
]]
