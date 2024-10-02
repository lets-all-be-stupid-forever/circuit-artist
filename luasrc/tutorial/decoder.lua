return {
  name='DECODER',
  text=[[

!img:imgs/tutorial/decoder1.png

A decoder transform an input binary number into a new binary number containing exactly 1 bit active, identifying the input.
Example:
`A`=0 (0) --> `D`=01 (1)
`A`=1 (1) --> `D`=10 (2)
!img:imgs/tutorial/decoder2.png
A 1-bit input decoder can be implemented as in the example above.
In order to have encoders with more inputs/outputs (example 2-bit input and 4-bit output), one trick you can do is to add an `ENABLE` input to the 1-bit decoder as follows:
!img:imgs/tutorial/decoder3.png
The enable flag basically makes the decoder work normally when E=1 (ie, the component is "enabled") and make it return all 0 when E=0 (ie, the component is "disabled").
Once you have this, you can compose them to create bigger decoders, as shown in the diagram below:
!img:imgs/tutorial/decoder4.png
The decoder allows you to easily identify which of the possible combination of inputs you have. For example, let's say you want your circuit to perform one kind of calculation for each possible value of input, then you can use a decoder to convert it to these "flag" formats and then use it as input to each calculation as "enables", allowing only the desired operation (the one with flag=1) to be performed.

  ]]
}
