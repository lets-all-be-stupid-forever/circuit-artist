require "_framework"

-- Port description
ports = {
  {name="a", dir='in', width=1},
  {name="b", dir='in', width=1},
  {name="a_nand_b", dir='out', width=1},
}

-- Test cases
cases = {
  {a=0, b=0, a_nand_b=1},
  {a=0, b=1, a_nand_b=1},
  {a=1, b=0, a_nand_b=1},
  {a=1, b=1, a_nand_b=0},
}


