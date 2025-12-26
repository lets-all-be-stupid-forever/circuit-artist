easyAddTest({
  cases = {
    {A_in=0, B_in=0, A_out_1=0, B_out_1=0, B_out_2=0, A_out_2=0,name='A=0 B=0'},
    {A_in=0, B_in=1, A_out_1=0, B_out_1=1, B_out_2=1, A_out_2=0,name='A=0 B=1'},
    {A_in=1, B_in=0, A_out_1=1, B_out_1=0, B_out_2=0, A_out_2=1,name='A=1 B=0'},
    {A_in=1, B_in=1, A_out_1=1, B_out_1=1, B_out_2=1, A_out_2=1,name='A=1 B=1'},
  },
  ports = {
    {name="A_in", width=1, input=false},
    {name="B_in", width=1, input=false},
    {name="A_out_1", width=1, input=true},
    {name="B_out_1", width=1, input=true},
    {name="B_out_2", width=1, input=true},
    {name="A_out_2", width=1, input=true},
  }
})
