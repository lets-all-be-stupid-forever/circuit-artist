comb_level[[
conn out 1 a
conn out 1 b
conn out 1 s
conn in 1 mux
test  0 0 0 0 a=0 b=0 s=0 --> mux=0(a)
test  0 1 0 0 a=0 b=1 s=0 --> mux=0(a)
test  1 0 0 1 a=1 b=0 s=0 --> mux=1(a)
test  1 1 0 1 a=1 b=1 s=0 --> mux=1(a)
test  0 0 1 0 a=0 b=0 s=1 --> mux=0(b)
test  0 1 1 1 a=0 b=1 s=1 --> mux=1(b)
test  1 0 1 0 a=1 b=0 s=1 --> mux=0(b)
test  1 1 1 1 a=1 b=1 s=1 --> mux=1(b)
]]
