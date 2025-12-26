easyAddTest({
  cases = {
    {A_in=bits("0000"), A_out=bits("0000") , name="A=0"},
    {A_in=bits("0001"), A_out=bits("0001") , name="A=1"},
    {A_in=bits("0010"), A_out=bits("0010") , name="A=2"},
    {A_in=bits("0011"), A_out=bits("0011") , name="A=3"},
    {A_in=bits("0100"), A_out=bits("0100") , name="A=4"},
    {A_in=bits("0101"), A_out=bits("0101") , name="A=5"},
    {A_in=bits("0110"), A_out=bits("0110") , name="A=6"},
    {A_in=bits("0111"), A_out=bits("0111") , name="A=7"},
    {A_in=bits("1000"), A_out=bits("1000") , name="A=8"},
    {A_in=bits("1001"), A_out=bits("1001") , name="A=9"},
    {A_in=bits("1010"), A_out=bits("1010") , name="A=10"},
    {A_in=bits("1011"), A_out=bits("1011") , name="A=11"},
    {A_in=bits("1100"), A_out=bits("1100") , name="A=12"},
    {A_in=bits("1101"), A_out=bits("1101") , name="A=13"},
    {A_in=bits("1110"), A_out=bits("1110") , name="A=14"},
    {A_in=bits("1111"), A_out=bits("1111") , name="A=15"},
  },
  ports = {
    {name="A_in", width=4, input=false},
    {name="A_out", width=4, input=true},
  }
})
