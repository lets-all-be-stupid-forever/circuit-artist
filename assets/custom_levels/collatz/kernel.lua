Import "submitfw"


-- Port description
ports = {
  {name="n",      dir='in', width=8},
  {name="steps",  dir='out', width=8},
}

-- Test cases
cases = {
  {n=1,  steps=0  },
  {n=2,  steps=1  },
  {n=4,  steps=2  },
  {n=5,  steps=5  },
  {n=3,  steps=7  },
  {n=6,  steps=8  },
  {n=7,  steps=16 },
  {n=8,  steps=3  },
  {n=9,  steps=19 },
  {n=10  ,steps=6  },
  {n=11  ,steps=14 },
  {n=12  ,steps=9  },
  {n=13  ,steps=9  },
  {n=14  ,steps=17 },
  {n=15  ,steps=17 },
  {n=16  ,steps=4  },
  {n=27  ,steps=111 },
  {n=41  ,steps=109 },
  {n=100  ,steps=25  },
  {n=123  ,steps=46  },
}

