Import "shared/comb_level.lua"

comb_level[[
conn out 1 A
conn out 1 B
conn in 1 A_less_than_B
conn in 1 A_equal_B
conn in 1 A_greater_than_B
test 0 0 0 1 0
test 0 1 1 0 0
test 1 0 0 0 1
test 1 1 0 1 0
]]
