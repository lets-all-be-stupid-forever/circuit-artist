return {
  name='MUX',
  text=[[

!img:imgs/tutorial/mux1.png

A `MUX` is a like a bit `selector`: Given 2 input bits `a` and `b`, and a selection bit `s`, if `s` is 0, `MUX` returns the value of `a`, and if `s` is 1, `MUX` returns the value of `b`.
!img:imgs/tutorial/mux3.png
Muxes work like selectors (or a "switch"). They're useful for example when you have multiple calculations and want to select only one of the results. It's like an "if" statement in programming: IF S=0, RETURN A; ELSE IF S=1 RETURN B;.
!img:imgs/tutorial/mux2.png
A mux can be implemented as shown in the diagram above.
You can also create muxes with more inputs by combining muxes having fewer inputs:
!img:imgs/tutorial/mux4.png
  ]]
}
