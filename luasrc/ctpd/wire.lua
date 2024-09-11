return {
  name="Wires",
  text=[[

Wires are formed by connected non-black pixels. Wires can have 4 states, as displayed below:
- `ON` (lighter color)
- `OFF` (darker color)
- `UNDEFINED` (magenta color)
- `ERROR` (red color)
!img:imgs/circuitopedia/wires1.png
The `ON` state represents the logic 1, and the `OFF` state represents the logic 0. Wires state can be toggled (1->0 or 0->1) by clicking on it during simulation mode.
!img:imgs/circuitopedia/wires2.png
The `ERROR` wires only appear when an error occurs during simulation. There are 2 possible errors that can occur:
Error type 1: A wire that belongs to the output of more than one gate. Example below. Each wire can only be the output of maximum 1 gate.
!img:imgs/circuitopedia/simu_error1.png
Error type 2: When the wire state oscillates between different values without end. Example below.
!img:imgs/circuitopedia/simu_error_cycle.png
The `UNDEFINED` state means that we don't know the state of the wire. Every wire start with that state, with exception of wires that are not output of a NAND gate. Those wires are initialized as 0.
Below an example of how the wire states change when the simulation is first launched:
!img:imgs/circuitopedia/simu_example1.png
The width or shape of a wire is not important, nor its color, as long as the pixels are connected.
!img:imgs/circuitopedia/wires3.png
However, wires are allowed to cross. As a rule of thumb, straight wires only connect at T or L connections.
!img:imgs/circuitopedia/wire_cross.png
Wires are only connected via up/down/left/right pixel neighbours. Diagonal pixels are not considered connected. In the example below, in the first case you have 4 different wires, while in the second example you have a single wire.
!img:imgs/circuitopedia/wire_diagonal.png
]]
}
