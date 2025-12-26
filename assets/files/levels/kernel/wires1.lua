easyAddTest({
  cases = {
    {A_in=0, A_out=0, name="A_in=0 A_out=0"},
    {A_in=1, A_out=1, name="A_in=0 A_out=0"},
  },
  ports = {
    {name="A_in", width=1, input=false},
    {name="A_out", width=1, input=true},
  }
})
