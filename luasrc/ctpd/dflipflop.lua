return {
  name='D Flip Flop',
  text=[[

!img:imgs/circuitopedia/mem8.png
Sometimes you want the memory to be updated only once when the E is set to 1.

Then, you can perform your calculations however you want, have the `D` bit modified with the new (next) value of the storage without interfering the current storage/calculation. Then, the storage is only updated again whenever the E goes to 0 and then back again to 1 ! (ie, in the next `CYCLE`, creating a proper sequential mechanism)

!img:imgs/circuitopedia/mem9.png

This can be achieved with a `D flip flop`, that can be created a using D latches.

!img:imgs/circuitopedia/mem10.png

We call this behaviour a `rising edge-enabled` memory, in contrast with the previous `level-enabled` memory of `D Latches`.  Below a comparison between the two storage modes:
!img:imgs/circuitopedia/mem13.png


]]
}
