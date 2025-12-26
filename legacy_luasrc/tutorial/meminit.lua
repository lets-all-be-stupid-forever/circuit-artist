return {
  name="Memory Initialization",
  text=[[

`MEMORY INITIALIZATION:` Flip flops don't have an initial state: the latches start at an undefined state, so everytime you have a synchronous circuit, before doing any calculation you need to initialize your memory somehow.

That's why we often introduce an extra auxiliar bit called `POWER-ON-RESET` that is 1 for a few initial clock cycles, then become zero.

!img:imgs/tutorial/sync6.png
!img:imgs/tutorial/sync8.png

You can then augment your flip-flop to accept an extra `RESET` input as follows:

!img:imgs/tutorial/sync7.png
  ]]
}
