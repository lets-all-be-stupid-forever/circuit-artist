return {
  name='DEMUX',
  text=[[

!img:imgs/circuitopedia/demux1.png

A `DEMUX` is like a bit `router`: Given an input bit `a`, 2 outputs `Y0` and `Y1` and a selector bit `s`, if `s` is 0, then set `Y0` to the value of `a` and `Y1` to 0. Otherwise, if `s` is 1, set `Y0` to 0 and `Y1` to the value of `a`.
!img:imgs/circuitopedia/demux3.png
Demuxes work like routers: you have a wire and the selector bit chooses to which wire the input bit should go.
Example:
a=0, s=0 --> Y0 = 0, Y1 = 0
a=1, s=0 --> Y0 = 1, Y1 = 0
a=0, s=1 --> Y0 = 0, Y1 = 0
a=1, s=1 --> Y0 = 0, Y1 = 1
A demux can be implemented as follows:
!img:imgs/circuitopedia/demux2.png
You can also create demuxes with more outputs by combining demuxes having fewer inputs:
!img:imgs/circuitopedia/demux4.png

  ]]
}
