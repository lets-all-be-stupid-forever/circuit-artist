Import "shared/comb_level.lua"

comb_level([[
conn out 1 R
conn out 1 S
conn in 1 Q
test 0 0 2 Initial state: undefined value
test 1 0 0 R=1: Q <-- 0 (Reset)
test 0 0 0 R=0,S=0: Q unchanged
test 1 0 0 R=1: Q <-- 0 (Reset)
test 0 0 0 R=0,S=0: Q unchanged
test 0 1 1 S=1: Q <-- 1 (Set)
test 0 0 1 R=0,S=0: Q unchanged
test 0 1 1 S=1: Q <-- 1 (Set)
test 0 0 1 R=0,S=0: Q unchanged
test 1 0 0 R=1: Q <-- 0 (Reset)
test 0 0 0 R=0,S=0: Q unchanged
test 0 1 1 S=1: Q <-- 1 (Set)
test 0 0 1 R=0,S=0: Q unchanged
]])
