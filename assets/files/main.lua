print('Inside global lua!')

-- Putting level here

print('-------------------')
print('-- LUA LOADED -----')
print('-------------------')

--[[
function addcomb(id, name, group, deps, assets)
  if group == nil then
    group = 'custom'
  end
  if assets == nil then
    assets = {}
  end
  add_level({
    id=id,
    group=group,
    name=name,
    assets=assets,
    deps=deps,
    icon="levels/icons/" .. id .. ".png",
    description=loadtxt('levels/desc/' .. id .. '.txt'),
    content={
      "shared/classic.lua",
      "shared/preload_chip.lua",
      "shared/comb_level.lua",
      "levels/kernel/" .. id .. ".lua",
    }
  })
end
]]--

function easy_add_level(ctx)
  if ctx.group == nil then
    ctx.group = 'custom'
  end
  if ctx.assets == nil then
    ctx.assets = {}
  end
  if ctx.content == nil then
    ctx.content = {
      "shared/classic.lua",
      "shared/preload_chip.lua",
      "shared/comb_level.lua",
      "levels/kernel/" .. ctx.id .. ".lua",
    }
  end

-- loadtxt('levels/desc/' .. ctx.id .. '.txt'),
  add_level({
    id=ctx.id,
    group=ctx.group,
    name=ctx.name,
    assets=ctx.assets,
    deps=ctx.deps,
    icon="levels/icons/" .. ctx.id .. ".png",
    description=ctx.description,
    content=ctx.content,
    extra_text=ctx.extra_text,
  })
end

----------------------------------------------------
add_group({
  name="Unsorted / Sandbox",
  id="custom",
  icon='sandbox.png',
  desc=[[
Unsorted Levels.
]] })


easy_add_level({
  id="sandbox",
  name="Sandbox",
  group='custom',
  description=[[
Sandbox mode with a clock that triggers when circuit is idle.

No objective.
]],
extra_text= {
  {
    title="Inputs",
    text=[[
The `clock` input allows you to create synchronous circuits. It triggers `on` or `off` when the circuit gets idle (it won't update in a constant rate).

The `power_on_reset` input allows you to initialize memory when applicable. It stays on (`1`) for the 2 first clock cycles, then it becomes off (`0`) for the rest of the simulation.
]]
},
}
})

----------------------------------------------------
add_group({
  name="Tutorial I - Wires and Gates",
  id='basics1',
  icon='basics.png',
  desc=[[
Learn the fundamentals of digital circuits.

Start by connecting wires, then build your first logic gates from NAND - the universal building block.

By the end, you'll understand how all Boolean logic can be constructed from a single gate type.
  ]]})

easy_add_level({
  id="wires1",
  name="Wires",
  group='basics1',
  description=[[
- Connect wire `A_in` to wire `A_out`.
]],
extra_text= { { wiki="wires1", } }
})

easy_add_level({
  id="wires2",
  name="Crossing Wires",
  group='basics1',
  deps={'wires1'},
  description=[[
- Connect wire `A_in` to `A_out`
- Connect wire `B_in` to `B_out`
]],
extra_text= { { wiki="wires_crossing", } }
})


easy_add_level({
  id="wires3",
  name="Wires I/O",
  group='basics1',
  deps={'wires2'},
  description=[[
- Connect wire `A_in` to `A_out_1` and `A_out_2`.
- Connect wire `B_in` to `B_out_1` and `B_out_2`.
]],
extra_text= { { wiki="wires_io", } }
})


easy_add_level({
  id="wires4",
  name="Multi-Bit Wires",
  group='basics1',
  deps={'wires3'},
  description=[[
  - Connect the 4-bit wire `A_in` to `A_out`.
  ]],
  extra_text= {
    {
      title='Tip: Line Tool',
      text=[[
You can use the "line tool" to make the task easier. It allows you to draw multiple parallel wires at the same time. Please check the tool tooltip for more instructions.
      ]]
    },
    {
      title='Pin bit order',
      text=[[
For multi-bit pins, bit0 is the uppermost wire, bit1 is the one below it, and so on, for both input and output pins.
      ]]
    },
    { wiki="bit_order", } }
  })


  easy_add_level({
    id="nand",
    name="NAND Gate",
    group='basics1',
    deps={'wires1'},
    description=[[
- Compute the NAND operation of the input pins `a` and `b`.
    ]],
    extra_text= {
      {
        title='NAND Gate Example',
        img="levels/imgs/nand_example.png",
        scale=8,
      },
      {
        title='NAND Gate Picture',
        img="levels/imgs/nand_schema.png",
        scale=4,
      },
      { wiki="nand", }
    }
    })


  easy_add_level({
    id="not",
    name="NOT Gate",
    group='basics1',
    deps={'nand'},
    description=[[
- Invert the value of the input pin `a`.
    ]],
    extra_text= {
      {
        title='NOT Gate Picture',
        img="levels/imgs/not_picture.png",
        scale=4,
      },
      {
        title='NOT Gate Schema',
        img="levels/imgs/not_schema.png",
        scale=4,
      },
      { wiki="not", }
    }
    })

  easy_add_level({
    id="and",
    name="AND Gate",
    group='basics1',
    deps={'not'},
    description=[[
- Compute the AND operation of the input pins `a` and `b` and write it to `a_and_b` output pin.
    ]],
    extra_text= {
      {
        title='AND Gate Picture',
        img="levels/imgs/and_picture.png",
        scale=4,
      },
      {
        title='AND Gate Schema',
        img="levels/imgs/and_schema.png",
        scale=4,
      },
      { wiki="and", }
    }
    })


  easy_add_level({
    id="or",
    name="OR Gate",
    group='basics1',
    deps={'and'},
    description=[[
- Compute the OR operation of the input pins `a` and `b` and write it to `a_or_b` output pin.
    ]],
    extra_text= {
      {
        title='OR Gate Picture',
        img="levels/imgs/or_picture.png",
        scale=4,
      },
      {
        title='OR Gate Schema',
        img="levels/imgs/or_schema.png",
        scale=4,
      },
      { wiki="or", }
    }
    })

   easy_add_level({
     id="xor",
     name="XOR Gate",
     group='basics1',
     deps={'or'},
     description=[[
 - Compute the XOR operation of the input pins `a` and `b` and write it to `a_xor_b` output pin.
     ]],
     extra_text= {
       {
         title='XOR Gate Picture',
         img="levels/imgs/xor_picture.png",
         scale=4,
       },
       {
         title='XOR Gate Schema',
         img="levels/imgs/xor_schema.png",
         scale=4,
       },
       { wiki="xor", }
     }
     })


  easy_add_level({
    id="a_eq_b1",
    name="A equals B",
    group='basics1',
    deps={'xor'},
    description=[[
Given two `1`-bit inputs `a` and `b`, return 1 if they're equal, or 0 if they're different.
    ]],
    extra_text= {
      {
        title='tip',
        img="levels/imgs/a_eq_b1_tip.png",
        scale=4,
      }
    }
    })


easy_add_level({
    id="a_eq_b2",
    name="A equals B x32",
    group='basics1',
    deps={'a_eq_b1', 'wires4'},
    description=[[
Given two `32`-bit inputs `a` and `b`, return 1 if they're equal, or 0 if they're different.
    ]],
    extra_text= {
      {
        title='tip',
        text='A equals B if: `A0=B0` AND `A1=B1` AND `A2=B2` AND `A3=B3` ... AND `A31=B31`.'
      }
    }
    })

 ------------------------------------------------------------
 -- Second campaign: 7 seg
 ------------------------------------------------------------
 add_group({
   name="Tutorial II - 7 Segment Display",
   id="sevseg",
   icon='sevenseg.png',
   desc=[[
Apply your logic gate knowledge to build a practical circuit.

Learn about decoders and how to convert binary numbers into visual output - the same principle used in calculators and digital clocks.
 ]]})

easy_add_level({
    id="match7",
    name="7?",
    group='sevseg',
    deps={},
    description=[[
Set output to 1 if `n` is `7`, otherwise set it to 0.
    ]],
    extra_text= {
      {
        wiki='posint'
      }
    }
    })

easy_add_level({
    id="match7_or_23",
    name="7 or 23?",
    group='sevseg',
    deps={},
    description=[[
- Set output to 1 if `n` is `7`.
- Set output to 1 if `n` is `23`.
- Set output to 0 otherwise.
    ]],
    extra_text= {
    }
    })

easy_add_level({
    id="decoder1",
    name="1-Bit Decoder",
    group='sevseg',
    deps={},
    description=[[
Implement a 1:2 decoder that takes a 1-bit input `a` and activates exactly one of the two output bits based on the input value:

- a=0 -> output[0]=1, output[1]=0 (binary: 10)
- a=1 -> output[0]=0, output[1]=1 (binary: 01)

    ]],
    extra_text= {
      {
        wiki="decoder1"
      }
    }
    })


easy_add_level({
    id="decoder2",
    name="1-Bit Decoder with Enable",
    group='sevseg',
    deps={'decoder1'},
    description=[[

Implement a 1:2 decoder with enable.

The enable input is an additional control signal: when `enable=1` (ON), the decoder operates normally. When `enable=0` (OFF), all outputs are zero.

    ]],
    extra_text= {}
    })

easy_add_level({
    id="decoder_2bit",
    name="2-Bit Decoder",
    group='sevseg',
    deps={},
    description=[[
Implement a 2:4 decoder that takes a 2-bit input `a` and activates exactly one of the 4 output bits based on the input value:

- a=0 -> output=0001
- a=1 -> output=0010
- a=2 -> output=0100
- a=3 -> output=1000

    ]],
    extra_text= {
    }
    })

easy_add_level({
    id="decoder3",
    name="5-Bit Decoder",
    group='sevseg',
    deps={'decoder2'},
    description=[[
Implement a 5:32 decoder.
    ]],
    extra_text= {}
    })

easy_add_level({
    id="match_many",
    name="Is it one of these?",
    group='sevseg',
    deps={'decoder2'},
    description=[[
Set output to 1 if `n` is one of these:

- 1
- 3
- 5
- 6
- 15
- 21
- 24
- 31

Otherwise set it to 0.
    ]],
    extra_text= {
      {
        title="Tip",
        text="You can pick the 5:32 decoder from the previous level and select only the outputs you're interested in.",
      },
      {
        title="Table",
        text=[[
This is a `truth table`, it shows you the desired output for each of the possible inputs.

| n  | output |
-----------
| 0  | 0 |
| 1  | `1` |
| 2  | 0 |
| 3  | `1` |
| 4  | 0 |
| 5  | `1` |
| 6  | `1` |
| 7  | 0 |
| 8  | 0 |
| 9  | 0 |
| 10 | 0 |
| 11 | 0 |
| 12 | 0 |
| 13 | 0 |
| 14 | 0 |
| 15 | `1` |
| 16 | 0 |
| 17 | 0 |
| 18 | 0 |
| 19 | 0 |
| 20 | 0 |
| 21 | `1` |
| 22 | 0 |
| 23 | 0 |
| 24 | `1` |
| 25 | 0 |
| 26 | 0 |
| 27 | 0 |
| 28 | 0 |
| 29 | 0 |
| 30 | 0 |
| 31 | `1` |
---------

]]
      },


    }
    })


easy_add_level({
    id="sevenseg",
    name="Seven Segment Display",
    group='sevseg',
    deps={'decoder2'},
    description=[[
Given a 4-bit input `n`, display it in a 7-segment display as in the image below.
    ]],
    assets={
     'seven_seg_display.png',
    },
    extra_text= {
      {
        title='Seven Segments',
        img="levels/imgs/sevenseg1.png",
        scale=4,
    }
    }
    })

add_group({
   name="Tutorial III - Routing Bits",
   id='muxes',
   icon='routing.png',
desc=[[
Master data flow control with multiplexers and demultiplexers.

These components let you select, route, and broadcast signals - essential building blocks for CPUs, memory systems, and communication buses.
]]})


easy_add_level({
    id="mux_2_1",
    name="Multiplexer 2:1",
    group='muxes',
    deps={},
    description=[[
Create a "1-bit selector" component, where the result is:
- `a` when `s` is `0`
- `b` when `s` is `1`
]],
    extra_text= {
      {
        wiki="mux",
      },
      {
        title="MUX Schema",
        img="mux1.png",
        scale=4,
      },
      {
        title="MUX Sample Implementation",
        img="mux2.png",
        scale=4,
      },
      {
        title="MUX Analogy to Switches",
        img="mux3.png",
        scale=4,
      },
    }
    })

easy_add_level({
    id="mux_4_1",
    name="Multiplexer 4:1",
    group='muxes',
    deps={},
    description=[[
Create a "selector" component, where the result is:
- `a` when `s` is `0`
- `b` when `s` is `1`
- `c` when `s` is `2`
- `d` when `s` is `3`
]],
    extra_text= {
      {
        title="Larger Multiplexers ",
        text=loadtxt("tut_mux_larger.txt"),
      },
      {
        title="4:1 MUX Schema",
        img="mux4.png",
        scale=4,
      },
    }
    })

easy_add_level({
    id="mux_4_2",
    name="Multiplexer 4:2",
    group='muxes',
    deps={},
    description=[[
Create a "selector" component, where the result is:
- `a` when `s` is `0`
- `b` when `s` is `1`
]],
    extra_text= {}
    })

easy_add_level({
    id="bus2",
    name="2-Bit Data Bus",
    group='muxes',
    deps={},
    description=[[
You have 4 external "chips" C0, C1, C2 and C3, and each of the chips have input and output wires.

Each chip reads data from C_in and writes data to C_out.

The thing is, they need to communicate with each other. One way they can do that is by sending the output from `one` of those chips to all the others, then they can alternate who speaks at a given time.

For that, there will be a 2-bit selector signal `s` telling which chip will be talking, and your task is to send the `OUTPUT` data from that chip to the `INPUT` of all other chips (including the origin itself).

In other words, create a 4:1 MUX that broadcasts the selected chip's output to all chips' inputs.

]],
extra_text= {
  {
    title='Example',
    text=[[
Let's say we have:
- c0_out=0
- c1_out=1
- c2_out=2
- c3_out=3

Then, when `s=1`, we want to have:

- c0_in=1 `(c1_out)`
- c1_in=1 `(c1_out)`
- c2_in=1 `(c1_out)`
- c3_in=1 `(c1_out)`

]]

  },
  {
    title='Example II',
    img='bus_example.png',
    scale=4,
  }
}
})

easy_add_level({
    id="demux_1_2",
    name="Demux 1:2",
    group='muxes',
    deps={},
    description=[[
Create a 1:2 "router" component:
- When `s=0` -> `d0=a`, `d1=0`
- When `s=1` -> `d0=0`, `d1=a`
]],
extra_text= {
  {
    wiki='demux',
  },
  {
    title='Analogy with a MUX',
    text=[[A DEMUX is like a reversed MUX:
- MUX: N inputs -> 1 output (selector picks which input)
- DEMUX: 1 input -> N outputs (selector picks which output receives it)
    ]],
  },
  {
    title='Analogy with a Decoder',
    text="When the data input to the DEMUX equals 1, we have a regular decoder (where the 'selector' becomes the decoder's input).",
  },
  {
    title='1:2 Demux',
    img='demux1.png',
    scale=4,
  },
  {
    title='1:2 Demux Example',
    img='demux2.png',
    scale=4,
  },
  {
    title='1:2 Demux Analogy',
    img='demux3.png',
    scale=4,
  },
}
})

easy_add_level({
    id="demux_1_4",
    name="Demux 1:4",
    group='muxes',
    deps={},
    description=[[
Create a 1:4 "router" component:
- When `s=0` -> `d0=a` d1=0 d2=0 d3=0
- When `s=1` -> d0=0 `d1=a` d2=0 d3=0
- When `s=2` -> d0=0 d1=0 `d2=a` d3=0
- When `s=3` -> d0=0 d1=0 d2=0 `d3=a`
]],
extra_text= {
  {
    title='1:4 Demux Tip',
    img='demux4.png',
    scale=4,
  },
}
})


easy_add_level({
    id="demux_2_4",
    name="Demux 2:4",
    group='muxes',
    deps={},
    description=[[
Create a 2:4 "router" component:
- When `s=0` -> `d0=a` d1=0
- When `s=1` -> d0=0 `d1=a`
]],
extra_text= {
}
})

easy_add_level({
    id="router",
    name="2-Bit Data Router",
    group='muxes',
    deps={},
    description=[[
You have 4 external "chips" C0, C1, C2 and C3, and each of the chips have input and output wires.

Each chip reads data from C_in and writes data to C_out.

The task is to send data from one chip to another chip. There will be 2 selectors: one for origin and one for destination, then your task is to send the output of the origin to the input of the destination, all other chips should receive 0.

In other words, combine a MUX (to select source) and a DEMUX (to route to destination).

]],
extra_text= {
  {
    title='Example',
    text=[[
Let's say we have:
- c0_out=0
- c1_out=1
- c2_out=2
- c3_out=3

Then, when `origin=3` and `destination= 1`, we want to have:

- c0_in=0
- c1_in=3 `(c3_out)`
- c2_in=0
- c3_in=0

]]
  },
  {
    title='Example II',
    img='router_example.png',
    scale=4,
  }
}
})




-- !img:imgs/tutorial/mux1.png
--
-- A `MUX` is a like a bit `selector`: Given 2 input bits `a` and `b`, and a selection bit `s`, if `s` is 0, `MUX` returns the value of `a`, and if `s` is 1, `MUX` returns the value of `b`.
-- !img:imgs/tutorial/mux3.png
-- Muxes work like selectors (or a "switch"). They're useful for example when you have multiple calculations and want to select only one of the results. It's like an "if" statement in programming: IF S=0, RETURN A; ELSE IF S=1 RETURN B;.
-- !img:imgs/tutorial/mux2.png
-- A mux can be implemented as shown in the diagram above.
-- You can also create muxes with more inputs by combining muxes having fewer inputs:
-- !img:imgs/tutorial/mux4.png


-- basics2 = 'basics2'
-- add_group({name="Decoders", id=basics2})
-- addcomb("decoder1", "Decoder", basics2)
-- addcomb("demux", "Demux", basics2, {'decoder1'})
-- addcomb("decoder2", "Decoder With Enable", basics2, {'decoder1'})
-- addcomb("decoder3", "5:32 Decoder", basics2, {'decoder2'})
--
-- math1 = 'math1'
-- add_group({name="Math I", id=math1})
-- addcomb("halfadder", "Half Adder", math1)
-- addcomb("fulladder", "Full Adder", math1, {'halfadder'})
-- addcomb("adder4bit", "4 bits Adder", math1, {'fulladder'})
-- addcomb("subtractor", "A - B", math1, {'adder4bit'})
--
-- comparing = 'comparing'
-- add_group({name="Bits", id=comparing})
-- addcomb("a_eq_b1", "A=B I", comparing)
-- addcomb("a_eq_b2", "A=B II", comparing, {'a_eq_b1'})
-- addcomb("comparator1", "Comparator", comparing, {'a_eq_b1'})
-- addcomb("comparator2", "4 Bits Comparator", comparing,  {'comparator1'})
-- addcomb("comparator3", "4 Bits Comparator (Signed)", comparing, {'comparator2'})
--
--
-- shifting = 'shifting'
-- add_group({name="Shifting", id=shifting})
-- addcomb("shift1", "Bit Shifter", shifting )
-- addcomb("shift2", "Cyclic Bit Shifter", shifting,  {'shift1'})
-- addcomb("barrel1", "Bit Shifter", shifting, {'shift2'})
-- addcomb("barrel2", "Cyclic Shifter", shifting, {'barrel1'})


----------------------------------------------------
-- addcomb("mof3", "Multiple of 3")
-- addcomb("ram", "Basic Memory")
--
-- local sevenSegAssets = {
--   'levels/sevenseg/seven_seg_display.png',
-- }
-- addcomb("sevenseg", "7-Segment Display", nil, nil, sevenSegAssets)

tut_add_topic("basic", "Basics", "wiki/icon_basics.png");
tut_add_topic("gates", "Gates", "wiki/icon_gates.png");
tut_add_topic("math", "Math", "wiki/icon_math.png");

-- function add_wiki(topic, id, name)
--   tut_add_item(topic, id, name, loadtxt("wiki/" .. id .. "/" .. id .. ".txt"), "wiki/" .. id .. "/icon_" .. id ..".png");
-- end

function add_wiki(topic, id, name)
  local desc = "wiki/desc/" .. id .. ".txt"
  local icon = "wiki/icons/" .. id .. ".png"
  tut_add_item(topic, id, name, loadtxt(desc), icon)
end


add_wiki("basic", "tldr", "TLDR")
add_wiki("basic", "wires1", "Basic wires")
add_wiki("basic", "wires_crossing", "Crossing Wires")
add_wiki("basic", "wires_io", "Wires IO")
add_wiki("basic", "bit_order", "Bit Order")
add_wiki("basic", "posint", "Number Representation")

add_wiki("gates", "nand", "Nand Gate")
add_wiki("gates", "not", "Not Gate")
add_wiki("gates", "and", "And Gate")
add_wiki("gates", "or", "Or Gate")
add_wiki("gates", "xor", "Xor Gate")
add_wiki("gates", "decoder1", "Decoder")
add_wiki("gates", "mux", "Mux")
add_wiki("gates", "demux", "Demux")

add_wiki("math", "halfadder", "Half Adder")
add_wiki("math", "fulladder", "Full Adder")
add_wiki("math", "rcadder", "Ripple Carry Adder")
add_wiki("math", "bit_shifting", "Bit Shifting")
add_wiki("math", "barrel_shifter", "Barrel Shifter")
add_wiki("math", "logarithmic_shifter", "Logarithmic Shifter")

-- tut_add_item("basic", "wires1", "Basic wires", loadtxt("wiki/wires1/wires1.txt"), "wiki/wires1/icon_wires1.png");
-- tut_add_item("basic", "nand", "Nand Gates",    loadtxt("wiki/nand/nand.txt"), "wiki/basic/nand/icon_nand.png");
