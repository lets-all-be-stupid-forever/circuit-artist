require "_framework"

ports = {
  {name="a", dir='in', width=1},
  {name="b", dir='in', width=1},
  {name="a_and_b", dir='out', width=1},
}

cases = {
  {a=0, b=0, a_and_b=0},
  {a=0, b=1, a_and_b=0},
  {a=1, b=0, a_and_b=0},
  {a=1, b=1, a_and_b=1},
}
