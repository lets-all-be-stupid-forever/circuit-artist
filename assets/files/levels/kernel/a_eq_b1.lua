easyAddTest({
  cases = {
    {a=0, b=0, a_eq_b=1, name='0 EQUALS 0 = 1'},
    {a=1, b=0, a_eq_b=0, name='0 EQUALS 1 = 0'},
    {a=0, b=1, a_eq_b=0, name='1 EQUALS 0 = 0'},
    {a=1, b=1, a_eq_b=1, name='1 EQUALS 1 = 1'},
  },
  ports = {
    {name="a", width=1, input=false},
    {name="b", width=1, input=false},
    {name="a_eq_b", width=1, input=true},
  }
})

