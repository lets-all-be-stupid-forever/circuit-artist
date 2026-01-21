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
   id="sevseg1",
   icon='sevenseg.png',
   desc=[[
Apply your logic gate knowledge to build a practical circuit.

Learn about decoders and how to convert binary numbers into visual output - the same principle used in calculators and digital clocks.
 ]]})

easy_add_level({
    id="match7",
    name="7?",
    group='sevseg1',
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
    group='sevseg1',
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
    group='sevseg1',
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
    group='sevseg1',
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
    group='sevseg1',
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
    group='sevseg1',
    deps={'decoder2'},
    description=[[
Implement a 5:32 decoder.
    ]],
    extra_text= {}
    })

easy_add_level({
    id="match_many",
    name="Is it one of these?",
    group='sevseg1',
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
    group='sevseg1',
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
   id='routing1',
   icon='routing.png',
desc=[[
Master data flow control with multiplexers and demultiplexers.

These components let you select, route, and broadcast signals - essential building blocks for CPUs, memory systems, and communication buses.
]]})


easy_add_level({
    id="mux_2_1",
    name="Multiplexer 2:1",
    group='routing1',
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
    group='routing1',
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
    group='routing1',
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
    group='routing1',
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
    group='routing1',
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
    group='routing1',
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
    group='routing1',
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
    group='routing1',
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

add_group({
   name="Tutorial IV - Memory",
   id='memory1',
   icon='campaign_memory1.png',
desc=[[
Discover how circuits can remember.

So far, outputs have depended only on current inputs. In this tutorial, you'll learn to build circuits that can store information and change behavior over time.

Start with simple latches, progress to flip-flops and registers, and finish by building the NAND Processing Unit Prototype 0.
]]})

easy_add_level({
    id="latch_door",
    name="Door Lock System",
    group='memory1',
    deps={},
    assets={
     'levels/door/door_base.png',
     'levels/door/door_open.png',
     'levels/door/door_closed.png',
     'levels/door/door_lock_up.png',
     'levels/door/door_lock_down.png',
     'levels/door/door_unlock_up.png',
     'levels/door/door_unlock_down.png',
    },
    description=[[

Create a simple door lock system.

The system has 2 buttons:
- one to unlock the door (`unlock_door_button`)
- one to lock the door (`lock_door_button`)

Each button is connected to a wire that goes into the circuit. When the button is not being pressed, the wire value for the button is 0, and when the button is down (ie being pressed), the wire value is 1.

When the `lock_door_button` is pressed, the door should become locked, and should remain locked until the unlock button is pressed. When the `unlock_door_button` is pressed, the door should become unlocked, and should remain unlocked until the lock button is pressed.

The circuit should return whether the door is locked or not:
- `door_locked`=1  --> The door is LOCKED.
- `door_locked`=0  --> The door is UNLOCKED.

The initial value of `door_locked` is not important.

There's a mechanism in the door that doesn't allow both buttons to be pressed at the same time.

]],
extra_text= {
  {
    title="Door",
    img="levels/door/door_img1.png",
    scale=3,
  },
  {
    title="Think about it",
    text=[[
When both buttons are released (both inputs are 0), how does the circuit "know" what to output?

Try connecting two NOT gates in a loop: the output of the first feeds into the second, and the output of the second feeds back into the first. Now click on one of the wires to change its value - the circuit stabilizes with one wire at 0 and the other at 1. Click again to flip it. This is memory! But there's a problem: you can only change it by clicking, not via an input signal.

Now replace those NOT gates with NAND gates. Remember that NAND(x, 1) = NOT(x), so when one input is held at 1, a NAND behaves like NOT. But when you set that input to 0, the NAND outputs 1 regardless - this lets you "force" a wire to 1 via an input.

With two NAND gates in a loop, each with an extra input, you have a controllable memory cell: one input forces the output to 1, the other forces it to 0, and when both inputs are 1, the circuit remembers its previous state.
    ]]
  },
  {
    title="Hint",
    text=[[
Think of the lock button as "Set" (set door_locked to 1) and the unlock button as "Reset" (reset door_locked to 0). When neither button is pressed, the output should stay as it was.
    ]]
  },
  {wiki='srlatch'},
}
})

easy_add_level({
    id="dlatch",
    name="D Latch",
    group='memory1',
    deps={},
    description=[[

Create a D Latch - a circuit that can be "enabled" or "disabled".

When enabled (`E=1`), the output `Q` copies the input `D`.
When disabled (`E=0`), the output `Q` holds its last value, ignoring any changes to `D`.

]],
extra_text= {
  {
    title="Example",
    text=[[
Example:
- 1st input: D=0 E=1 --> Q=0
- 2nd input: D=1 E=1 --> Q=1
- 3rd input: D=0 E=0 --> Q=1 (Q doesn't change because E=0)
- 4th input: D=1 E=0 --> Q=1 (Q doesn't change because E=0)
- 5th input: D=0 E=1 --> Q=0 (Q changes again because E=1)
    ]]
  },
  {
    title="Analogy with the Door Level",
    text=[[
It's like the door lock level, but:
- You want to LOCK when D=1 and E=1 and UNLOCK when D=0 and E=1
- When E=0, you want to behave as if no button is pressed. This can be done with LOCK=0 and UNLOCK=0.
    ]]
  },
  {wiki='dlatch'},
  {
    title='Symbol',
    scale=4,
    img='wiki/mem2.png'
  },
  {
    title='Truth Table',
    scale=4,
    img='wiki/mem3.png'
  },
  {
    title='D Latch Implementation',
    scale=4,
    img='wiki/mem4.png'
  },

}
})

easy_add_level({
    id="photo",
    name="Bit Photo",
    group='memory1',
    deps={},
    description=[[

!img:levels/camera_img1.png

Build a simple camera that takes photos of a bit.

The camera has:
- A `sensor` that receives light non-stop, and sends the light value to the 1-bit input of the circuit.
- A "take photo" button (`take_photo_button_pressed`), that can be pressed (=1) or not pressed (=0).
- A 1-bit output display (`photo`).

Whenever the user presses the button, the output should capture the bit value present at the sensor at that EXACT moment. When the user releases the button, the display should still show the last taken photo. When the user presses the button again, a new photo should be taken.

The sensor value may change while the user is still pressing the button, but even if it does, the output should not change: it should capture the value of the sensor at the EXACT moment the button is pressed.

At the exact moment the user presses the button, the sensor will not change.

]],
extra_text= {
  {
    title='The Problem with D Latch',
    text=[[
You might think: "I'll use a D Latch with the button as Enable and the sensor as D."

But there's a problem: while the button is held down (E=1), the output keeps following the sensor. If the sensor changes while you're holding the button, the "photo" changes too!

You need the output to capture the sensor value at the EXACT moment the button is pressed, then hold that value even if the sensor changes afterward.
    ]]
  },
  {
    title='Rising Edge Detection',
    text=[[
The key insight is that you need to detect the "rising edge" - the exact moment the button goes from 0 to 1.

A D Latch updates continuously while enabled. A `D Flip-Flop` updates only once, at the rising edge of the clock signal.

The button press IS your clock signal! When the button goes from 0 to 1, that rising edge should capture the sensor value.
    ]]
  },
  {
    title='Building a D Flip-Flop',
    text=[[
A D Flip-Flop can be built using two D Latches in a "master-slave" configuration:

1. The first latch (master) is enabled when CLK=0
2. The second latch (slave) is enabled when CLK=1
3. Connect the output of master to the input of slave

When CLK rises (0→1): master locks its value, slave captures it. When CLK is high: master ignores D, slave holds steady. The result: output only changes at the rising edge!
    ]]
  },
  {
    title='Example',
    img='levels/camera_img2.png',
    scale=4,
  },
  {
    wiki='dflipflop'
  },
  {
    wiki='synchronous'
  },
}
})

easy_add_level({
    id="combo_detector",
    name="Combo Detector",
    group='memory1',
    deps={},
    description=[[

Build a circuit that detects if the last 4 button presses were all button A.

**Inputs:**
- `button_A`: 1 when button A is being pressed, 0 otherwise
- `button_B`: 1 when button B is being pressed, 0 otherwise

Only one button can be pressed at a time (never both). When no button is pressed, both inputs are 0.

**Output:**
- `combo_detected`: 1 if the last 4 button presses were all button A, 0 otherwise

The first 4 outputs are not checked (the system needs 4 presses to have enough history).

]],
extra_text= {
  {
    title='Creating a Clock Signal',
    text=[[
Unlike previous levels, this level has no external clock input. Instead, you need to create your own clock signal from the button inputs.

A "button press" happens when either button goes from 0 to 1. You can detect this with:

`clock = button_A OR button_B`

This clock ticks whenever any button is pressed.
    ]]
  },
  {
    title='The Timing Problem',
    text=[[
If you derive the clock from the buttons and connect it directly to your flip-flops, you'll have a problem: the clock and the data (which button was pressed) arrive at the same time!

For a flip-flop to work correctly, the data input must be stable BEFORE the clock rising edge arrives. When both come from the same source, there's a race condition.
    ]]
  },
  {
    title='Clock Delay Solution',
    text=[[
The solution is to delay the clock signal so it arrives AFTER the data is stable.

You can add delay using a chain of NOT gates: `NOT(NOT(signal))` gives you the same value but delayed by 2 gate updates.

So your circuit should:
1. Use `button_A` directly as the data to store (1 if A was pressed, 0 if B was pressed)
2. Create the clock as `button_A OR button_B`
3. Delay the clock through NOT-NOT chains before sending it to your flip-flops
    ]]
  },
  {
    title='Storing History',
    text=[[
You need to remember the last 4 button presses. Use 4 flip-flops chained together:
- FF1 stores the most recent press
- FF2 stores the press before that
- FF3 stores the one before that
- FF4 stores the oldest of the 4

On each clock tick, shift the values: FF4 <- FF3 <- FF2 <- FF1 <- new_input
    ]]
  },
  {
    title='Computing the Output',
    text=[[
Once you have the last 4 presses stored, the output is simple:

`combo_detected = FF1 AND FF2 AND FF3 AND FF4`

If all 4 stored values are 1 (meaning all 4 presses were button A), output 1. Otherwise output 0.
    ]]
  },
  {
    wiki='propdelay'
  },
  {
    wiki='setuphold'
  },
}
})

easy_add_level({
    id="dflipflop_with_enable",
    name="D Flip-Flop With Enable",
    group='memory1',
    deps={},
    description=[[

Create a D Flip-Flop with enable (`E`). The enable tells the flip-flop to NOT store `D` when the clock reaches the rising edge.

When `E`=1, the flip flop should work normally, updating its stored value.

When `E`=0, the flip flop should ignore the input (by updating with its previous value instead).

]],
extra_text= {
  {
    title='Hint',
    text=[[
In the flip flop input, you can use the `E` signal to select between the flip flop's output (`Q`) and the `D` input.
    ]]
  },
  {
    title='Example',
    text=[[
- D=0 E=1 CLK=rising(0 to 1) --> Q =0
- D=1 E=1 CLK=rising(0 to 1) --> Q =1
- D=0 E=1 CLK=rising(0 to 1) --> Q =0
- D=0 E=0 CLK=rising(0 to 1) --> Q =0
- D=1 E=0 CLK=rising(0 to 1) --> Q =0 (no change)
- D=0 E=0 CLK=rising(0 to 1) --> Q =0
- D=1 E=1 CLK=rising(0 to 1) --> Q =1 (change)
    ]]
  },
  {
    title='Test Cases',
    text=[[
  CLK D E Q
#00 1 0 1 2
#01 0 0 1 2
#02 1 0 1 0
#03 1 0 1 0
#04 0 1 1 0
#05 1 1 1 1
#06 0 0 0 1
#07 1 0 0 1
#08 0 1 0 1
#09 1 1 0 1
#10 0 0 1 1
#11 1 0 1 0
#12 0 1 1 0
#13 1 1 1 1
#14 0 0 1 1
#15 1 0 1 0
#16 0 0 0 0
#17 1 0 0 0
#18 0 1 0 0
#19 1 1 0 0
#20 0 1 1 0
#21 1 1 1 1
#22 0 0 0 1

    ]]
  },
  {
    title='Comment',
    text=[[
The Enable signal is important for when you only want to update a few of your flip flops. For example, if you store a "memory" of 100 bits and just want to update a few you set most of them to E=0 and the one you want to change to E=1.
    ]]
  },
  {
    title='Schema',
    img='levels/D_with_enable.png',
    scale=4,
  },
  {
    wiki='dflipflop'
  },
}
})

easy_add_level({
    id="dff_w_r",
    name="D FF With Reset",
    group='memory1',
    deps={},
    description=[[

Create a D Flip-Flop with enable (`E`) and reset (`RST`). When the reset is on and the clock reaches rising edge (0->1), the value of the flip flop (`Q`) should become 0, independent of the value of `D` and `E`.

When `RST`=0, the flip flop should work normally (as a FF with enable).

When `RST`=1, the FF value should be set to 0.

]],
extra_text= {
  {
    title='Hint',
    text=[[
In the same place where the `E` signal chooses between `D` and `Q`, the `RST` signal can choose between those and `0` as input to the D-Flip Flop.
    ]]
  },
  {
    title='Comment',
    text=[[
The Reset signal is important at startup for example to initialize all your "data" to zero.
    ]]
  },
  {
    title='Schema',
    img='levels/D_with_reset.png',
    scale=4,
  },
  {
    title='Test Cases',
    text=[[
  CLK D E RST Q
#00 1 0 1 0 2
#01 0 0 1 0 2
#02 1 0 1 0 0
#03 1 0 1 0 0
#04 0 1 1 0 0
#05 1 1 1 0 1
#06 0 0 0 0 1
#07 1 0 0 0 1
#08 0 1 0 1 1
#09 1 1 0 1 0 Reset
#10 0 0 1 0 0
#11 1 0 1 0 0
#12 0 1 1 0 0
#13 1 1 1 0 1
#14 0 0 1 0 1
#15 1 0 1 0 0
#16 0 0 0 1 0
#17 1 0 0 1 0 Reset
#18 0 1 0 1 0
#19 1 1 0 1 0 Reset
#20 0 1 1 0 0
#21 1 1 1 0 1
#22 0 0 0 1 1
#23 1 1 1 1 0 Reset
#24 0 1 1 0 0
#25 1 1 1 0 1
#26 0 0 0 1 1
#27 1 0 0 1 0 Reset
#28 0 0 0 0 0
    ]]
  },
  {
    wiki='dflipflop'
  },
}
})

easy_add_level({
    id="register4",
    name="4-bit Register",
    group='memory1',
    deps={},
    description=[[

Create a 4-bit register with Enable and Reset.

A Register is simply a group of n flip-flops sharing the same clock, and they update simultaneously.

**Inputs:**
- `CLK`: Clock signal
- `Data`: 4-bit input value
- `E`: Enable (when 1, register updates; when 0, register keeps its value)
- `RST`: Reset (when 1, register resets to 0000, regardless of E)

**Output:**
- `Q`: 4-bit stored value

Reset takes priority over Enable: if `RST`=1, the register resets to 0 even if `E`=0.

]],
extra_text= {
  {
    title='Hint',
    text=[[
Use 4 D Flip-Flops with Enable and Reset. Connect the same CLK, E, and RST signals to all 4 flip-flops. Each flip-flop handles one bit of Data and produces one bit of Q.
    ]]
  },
  {
    title='Test Cases',
    text=[[
  CLK Data E RST Q
#00 1 0000 1 1 2222
#01 0 1010 1 1 2222
#02 1 1010 1 1 0000 E=1 RST=1
#03 0 1001 1 0 0000
#04 1 1001 1 0 1001 E=1 RST=0
#05 0 1100 0 0 1001
#06 1 1100 0 0 1001 E=0 RST=0
#07 0 0011 0 1 1001
#08 1 0011 0 1 0000 E=0 RST=1
#09 0 0110 1 0 0000
#10 1 0110 1 0 0110
#11 0 1111 1 0 0110
#12 1 1111 1 0 1111
    ]]
  },
  {
    wiki='dflipflop'
  },
}
})


easy_add_level({
    id="registerfile",
    name="Register File 4 x 1-bit",
    group='memory1',
    deps={},
    description=[[

Create a 4 x 1-bit register file.

A register file is a small, fast memory that holds multiple registers. Unlike a single register where you always write to the same place, a register file lets you choose which register to write to using an address.

**Inputs:**
- `rst`: Reset (sets all registers to 0 on rising edge)
- `clk`: Clock signal
- `data`: 1-bit value to write
- `waddr`: 2-bit write address (selects which register to write: 0, 1, 2, or 3)
- `write_enable`: When 1, writes `data` to the register selected by `waddr` on rising edge

**Outputs:**
- `y0`: Current value of register 0
- `y1`: Current value of register 1
- `y2`: Current value of register 2
- `y3`: Current value of register 3

All 4 register values are always visible at the outputs. The `waddr` only controls which one gets updated when writing.

]],
extra_text= {
  {
    title='Hint',
    text=[[
Use 4 D Flip-Flops with Enable and Reset - one for each register. The key is routing the `write_enable` signal to only the selected flip-flop.

Think about it: you need to enable exactly one of the 4 flip-flops based on `waddr`. A demultiplexer (demux) does exactly this - it routes a single input to one of several outputs based on a selector.

Use a 1:4 demux to route `write_enable` to the correct flip-flop's enable input.
    ]]
  },
  {
    title='Test Cases',
    text=[[
  rst clk data waddr we -> y0 y1 y2 y3
#00 1   0   0    00   0     ?  ?  ?  ? Power On
#01 1   1   0    00   0     0  0  0  0 Reset
#02 0   0   1    00   1     0  0  0  0
#03 0   1   1    00   1     1  0  0  0 y0 <-- 1
#04 0   0   1    01   1     1  0  0  0
#05 0   1   1    01   1     1  1  0  0 y1 <-- 1
#06 0   0   0    00   1     1  1  0  0
#07 0   1   0    00   1     0  1  0  0 y0 <-- 0
#08 0   0   1    11   1     0  1  0  0
#09 0   1   1    11   1     0  1  0  1 y3 <-- 1
#10 0   0   1    10   0     0  1  0  1
#11 0   1   1    10   0     0  1  0  1 no op (we=0)
    ]]
  },
  {
    wiki='dflipflop'
  },
  {
    wiki='demux'
  },
}
})

easy_add_level({
    id="npu1",
    name="NPU-P0",
    group='memory1',
    deps={},
    description=[[
Create the NAND Processing Unit Prototype 0 (NPU-P0).

Until now, circuits performed a fixed computation: given the same inputs, they always do the same thing. The NPU-P0 is different. It reads an operation code (`op`) each clock cycle and performs the corresponding action. By feeding it a sequence of operations, you can make it compute things that no fixed circuit could achieve.

The NPU-P0 should have 4 registers (R0, R1, R2, R3) for storage and should support 4 operations. The `op` input selects which operation to perform, while other inputs specify parameters like which register to target or what value to load.

**Inputs:**
- `rst`: Reset signal (should set all registers to 0 on rising edge)
- `clk`: Clock signal (operations should execute on rising edge)
- `op`: 2-bit operation code
- `data`: 1-bit value to load directly into a register (used by LOAD)
- `reg1`: 2-bit register selector (destination)
- `reg2`: 2-bit register selector (source)

**Operations (should be active on clock rising edge):**
- `op=0` NOP: Do nothing
- `op=1` LOAD: Load the `data` bit into register `reg1`
- `op=2` MOV: Copy the value of register `reg2` into register `reg1`
- `op=3` NAND: Compute R0 NAND R1 and store result in R0

**Output:**
- `y`: Should output the current value of R3
]],
extra_text= {
  {
    title='Architecture Hint',
    text=[[
Think of the NPU-P0 as combining several components you've already built:

1. **Register File**: 4 flip-flops to store R0-R3
2. **Read Logic**: Muxes to select register values based on `reg1` and `reg2`
3. **Write Logic**: Compute what value to write based on `op`:
   - NOP: don't write anything (disable all registers)
   - LOAD: write `data`
   - MOV: write value of register `reg2`
   - NAND: write R0 NAND R1 (only to R0)
4. **Write Routing**: Enable only the correct register to update

The tricky part is computing the write enable for each register based on `op` and `reg1`.
    ]]
  },
  {
    title="Note on 'data' input",
    text=[[
The `data` input provides a constant value that gets loaded directly into a register. In CPU terminology, this kind of constant is often called an "immediate" value, because the value is immediately available in the instruction itself, rather than being fetched from a register or memory.
    ]]
  },
  {
    title="NPU-P0 Assembly Reference",
    text=[[
The NPU-P0 understands 4 instructions:

`NOP`
  Do nothing. Registers remain unchanged.
  Example: NOP

`LOAD Rx, value`
  Load a constant (0 or 1) into register Rx.
  Example: LOAD R0, 1  ; R0 becomes 1
  Example: LOAD R2, 0  ; R2 becomes 0

`MOV Rx, Ry`
  Copy the value from register Ry into register Rx.
  Example: MOV R1, R0  ; R1 becomes the value of R0
  Example: MOV R3, R2  ; R3 becomes the value of R2

`NAND`
  Compute R0 NAND R1 and store the result in R0.
  Example: If R0=0 and R1=1, after NAND: R0 becomes 1 (because NAND(0,1)=1), R1 remains 1.
  Example: If R0=1 and R1=1, after NAND: R0 becomes 0 (because NAND(1,1)=0), R1 remains 1.
    ]]
  },
  {
    title="Test Program: AND.asm",
    text=[[
; =============================
; AND.asm
; Platform: NPU-P0
; Computes R3 = A AND B
; Input: A, B (constants)
; Output: R3
; =============================

`LOAD R0, A`       ; R0 = A
`LOAD R1, B`       ; R1 = B
`MOV R2, R0`       ; R2 = A (backup)
`MOV R3, R1`       ; R3 = B (backup)
`NAND`             ; R0 = A NAND B
`MOV R1, R0`       ; R1 = A NAND B
`NAND`             ; R0 = (A NAND B) NAND (A NAND B) = A AND B
`MOV R3, R0`       ; R3 = result

; The test runs this program 4 times with:
; A=0 B=0 -> R3=0
; A=0 B=1 -> R3=0
; A=1 B=0 -> R3=0
; A=1 B=1 -> R3=1
    ]]
  },
}
})


-- easy_add_level({
--     id="doorpwd",
--     name="Door Lock With Password",
--     group='memory1',
--     deps={},
--     description=[[
--
-- Create a door lock system with password. Instead of a unlock button, the door has now 3 buttons:
--
-- - A `0` button.
-- - A `1` button.
-- - A `lock` button.
--
-- At any time the door can be locked or unlocked. The door password is known beforehand: 00101101, which means that once the user enters the code in this exact sequence the door should unlock and remain unlocked, until he presses again the "lock button", which also resets the previous entered code values.
--
-- So:
--
-- - When user enters 0 -> 0 -> 1 -> 0 -> 1 -> 1 -> 0 -> 1, the door should unlock. (and remain unlocked unless the user presses the "lock" button)
-- - When user presses "lock", the door should unlock and the system works as if the user had previously typed `0` 100 times (ie the "history" becomes all 0).
--
-- The tests start with the user locking the door (ie pressing the lock button).
--
-- Again, here the user can only press one button at a time, and the user presses the buttons significantly slower between one press and the next (ie, there won't be "pressing too fast" scenarios).
--
-- PS: It's ok for it to lock/unlock only when user releasees the button.
--
-- ]],
-- extra_text= {
--   {
--     title="Tip",
--     text=[[
-- You'll need to introduce a small time delay between inputs and your clock signal: the clock signal must arrive after the input signals.
-- ]]
--   }
-- }
-- })



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
tut_add_topic("mem", "Sequential Circuits", "wiki/icon_memory.png");
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
add_wiki("basic", "hotkeys", "Hotkeys")
add_wiki("basic", "wires1", "Basic wires")
add_wiki("basic", "wires_crossing", "Crossing Wires")
add_wiki("basic", "wires_io", "Wires IO")
add_wiki("basic", "bit_order", "Bit Order")
add_wiki("basic", "posint", "Number Representation")
add_wiki("basic", "customlevel", "Custom Level")

add_wiki("gates", "nand", "Nand Gate")
add_wiki("gates", "not", "Not Gate")
add_wiki("gates", "and", "And Gate")
add_wiki("gates", "or", "Or Gate")
add_wiki("gates", "xor", "Xor Gate")
add_wiki("gates", "decoder1", "Decoder")
add_wiki("gates", "mux", "Mux")
add_wiki("gates", "demux", "Demux")

add_wiki("mem", "srlatch", "SR Latch")
add_wiki("mem", "dlatch", "D Latch")
add_wiki("mem", "dflipflop", "D Flip Flop")
add_wiki("mem", "synchronous", "Synchronous Circuits")
add_wiki("mem", "meminit", "Memory Initialization")
add_wiki("mem", "propdelay", "Propagation Delay")
add_wiki("mem", "setuphold", "Setup and Hold Time")

add_wiki("math", "halfadder", "Half Adder")
add_wiki("math", "fulladder", "Full Adder")
add_wiki("math", "rcadder", "Ripple Carry Adder")
add_wiki("math", "bit_shifting", "Bit Shifting")
add_wiki("math", "barrel_shifter", "Barrel Shifter")
add_wiki("math", "logarithmic_shifter", "Logarithmic Shifter")

-- tut_add_item("basic", "wires1", "Basic wires", loadtxt("wiki/wires1/wires1.txt"), "wiki/wires1/icon_wires1.png");
-- tut_add_item("basic", "nand", "Nand Gates",    loadtxt("wiki/nand/nand.txt"), "wiki/basic/nand/icon_nand.png");
