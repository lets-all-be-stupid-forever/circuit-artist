require "_framework"

ports = {
  {name="a", dir='in', width=1},
  {name="not_a", dir='out', width=1},
}

cases = {
  {a=0, not_a=1},
  {a=1, not_a=0},
}
