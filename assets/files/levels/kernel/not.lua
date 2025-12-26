easyAddTest({
  cases = {
    {a=0, not_a=1, name='NOT 0 = 1'},
    {a=1, not_a=0, name='NOT 0 = 0'},
  },
  ports = {
    {name="a", width=1, input=false},
    {name="not_a", width=1, input=true},
  }
})


