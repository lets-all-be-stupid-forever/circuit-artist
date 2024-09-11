return {
  name="A+B",
  text=[[

In this section we will see how to `add` two integers of `N` bits (signed or unsigned).

First, we have to create a sub-circuit that perform the addition of two bits, and outputs both the result, and an extra "carry", as you would do when summing numbers by hand. We call this sub-circuit a `HALF ADDER`.
!img:imgs/circuitopedia/addition1.png
That can be created as follows: it's just a XOR for the addition, with an extra AND to add a carry whenever both inputs are 1 (only situation where we have a carry=1).
!img:imgs/circuitopedia/addition2.png
Having the carry as output is not enough. Now we need to extend the subcircuit to accept a carry input as well, as we do when we are summing numbers by hand. This extended sub-circuit is called a `FULL ADDER`, and can be created as follows. Basically instead of summing `a` and `b`, we will sum `a`, `b` and `carry_in` bits.
!img:imgs/circuitopedia/addition3.png
Once we have the full adder, we can combine them together to create an adder for N-bits, as follows:
!img:imgs/circuitopedia/addition4.png

  ]]
}
