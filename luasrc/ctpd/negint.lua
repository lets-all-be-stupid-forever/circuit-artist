return {
  name="Signed Numbers",
  text=[[

For `signed integers`, we create a sort of extension of the unsigned integer definition:
!img:imgs/circuitopedia/numbers2.png
This representation is also called `two's complement`.

The formula to convert a number to its negative is given as: `-N = NOT(N) + 1`. It works for both positive and negative numbers.

So let's say you want to find -3 in a 4-bit representation. You know that 3 is represented in positive integers by 0011. So, in order to find -3 first you find NOT(3) by inverting each bit, getting 1100. Then, you add 1 to it, getting 1100 + 1 = 1101, so the representation of -3 is 1101. You can also do the other way around, to get -(-3): First you invert 1101 to get 0010, then you add 1, to get 0010 + 1 = 0011, which is our starting representation of 3.

]]
}
