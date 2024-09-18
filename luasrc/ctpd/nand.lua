return {
  name="NAND Gate",
  text=[[

!img:imgs/circuitopedia/nand1.png

A NAND logic gate is represented by 3 pixels, as shown below.

!img:imgs/circuitopedia/nand0.png

A logic gate is a "mechanism" that transforms one or two bit inputs into one bit output.

The NAND logic gate has 2 inputs and 1 output. It "reads" the value of the inputs and, depending on their value, it will assign a value to the output wire. The assignment table can be found below:

a=0 b=0 --> NAND=1
a=0 b=1 --> NAND=1
a=1 b=0 --> NAND=1
a=1 b=1 --> NAND=0

For example, if the 2 inputs are 0, then the output will be assigned to 1. See examples below.

!img:imgs/circuitopedia/nand2.png

The NAND gates can be drawn in any orientation (facing up, down, left or right):

!img:imgs/circuitopedia/nand3.png

Every NAND should have its 2 inputs present, as well as the output, otherwise it will trigger an error when the simulation is launched.

!img:imgs/circuitopedia/nand4.png

Every non-black pixel in the image that is not part of a NAND gate is considered a wire, and has some state associated to it, as described below.

  ]]
}
