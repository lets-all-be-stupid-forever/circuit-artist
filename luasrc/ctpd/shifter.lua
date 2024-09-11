return {
  name="Barrel Shifter",
  text=[[

In this section we will see how to `shift` bits in a number.

The `shift` operation is simply moving the bits either to the left or to the right, as in the example below:

!img:imgs/circuitopedia/shift1.png

The shift operation is often represented by `A << b` when `A` is shifted by `b` bits to the left and `A >> b` when it is shifted to the right.

You can solve it by using a grid-like structure called `barrel shifter`. In order to understand how it works, let's look at a 4-bits right shifter `C = A >> B`. We would want the circuit for each shift value `B`  to look more or less like the following:

!img:imgs/circuitopedia/shift3.png

This can be achieved by creating a cell, then reproduce it in a grid-like structure like the picture below, then you create activation bit flags for each possible shift and link them to the associate cells. The function of each cell is to create the "L"-like curves as in the circuit above.

!img:imgs/circuitopedia/shift2.png

We can build each cell the following way:

!img:imgs/circuitopedia/shift4.png


]]
}
