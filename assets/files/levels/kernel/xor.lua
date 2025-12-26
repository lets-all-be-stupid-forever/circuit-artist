easyAddTest({
  cases = {
    {a=0, b=0, a_xor_b=0, name='0 XOR 0 = 0'},
    {a=1, b=0, a_xor_b=1, name='0 XOR 1 = 1'},
    {a=0, b=1, a_xor_b=1, name='1 XOR 0 = 1'},
    {a=1, b=1, a_xor_b=0, name='1 XOR 1 = 0'},
  },
  ports = {
    {name="a", width=1, input=false},
    {name="b", width=1, input=false},
    {name="a_xor_b", width=1, input=true},
  }
})
