return {
  name='D Latch',
  text=[[

!img:imgs/tutorial/mem2.png
We can extend the SR latch to create a `D`-Latch, where we have two inputs: a data input `D`, containing the bit to be assigned to the memory, and an `E` enable input, which will tell the memory to update or not (when E=0, the bit is not assigned and nothing happens). The output `Q` will represent the stored bit.
!img:imgs/tutorial/mem3.png
It can be implemented as follows:
!img:imgs/tutorial/mem4.png
As an example on how the output `Q` of the D-Latch changes over time when `E` and `D` changes.

Mind that Q can change at anytime whenever `E`=1. This can be inconvenient sometimes. Imagine for example, that you pick the stored bit, do some calculation on it and want to store it again. If the E keeps active, the bit will be updated immediately, which will trigger the calculation again!

]]
}
