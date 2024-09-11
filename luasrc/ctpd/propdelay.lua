return {
  name="Propagation Delay",
  text =[[

`PROPAGATION DELAY:` You should avoid adding gates between the clock signal and the input of flip flops, otherwise your memory will no longer be synchronous and you might have bugs related to some memory being update after the others.

!img:imgs/circuitopedia/sync5.png

However there might be some cases where one would want to create `gated clocks`, namely to reduce NAND updates on unused parts. That might help reduce total update count and speed up simulation. In real circuits, that would be equivalent to consuming less power.

In those cases, one should plan thoroughly to avoid asynchronous flip flop updates, minding that every NAND has the same update delay of 1 time unit.

]]
}
