return {
  name="A>B",
  text=[[

The objective of this section is to compare two positive integer numbers `A` and `B`. We often want to do the 3 comparisons at a time, (i) `A > B`, (ii) `A = B` and (iii) `A < B`.

We start creating a comparator for a single bit:
  !img:imgs/tutorial/comparator1.png
Which can be done as follows:
  !img:imgs/tutorial/comparator2.png
  !img:imgs/tutorial/comparator3.png
Then, in a second step, we extend this 1-bit comparator to accept inputs from a "more significant" bit. The idea is to do it the same way we do to compare two numbers: first we check if the most significant digit is equal, lower or higher than the other: if it's lower or higher we know whether a number is lower or higher, but if they're equal, we need to move forward to the next digit, creating a sequence of comparisons.
  !img:imgs/tutorial/comparator4.png
The formulas will look like the following, where `Aprv` = previous A, ie, result from previous comparator (from a more signficant bit), and `Anxt` = next A, ie, result going to next comparator (towards a least significant bit).

`A`<`Bnxt` = (`A`<`Bprv`) OR (`A`=`Bprv` AND `A`<`B`)
`A`=`Bnxt` = (`A`=`Bprv`) AND (`A`=`B`)
`A`>`Bnxt` = (`A`>`Bprv`) OR (`A`=`Bprv` AND `A`>`B`)

Then, similarly to the addition, we chain these comparators to create a bigger comparator for N bits.
  !img:imgs/tutorial/comparator5.png
  ]]
}
