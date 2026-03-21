
function add_wiki(topic, id, name)
  local desc = "wiki/desc/" .. id .. ".txt"
  local icon = "wiki/icons/" .. id .. ".png"
  tut_add_item(topic, id, name, loadtxt(desc), icon)
end

tut_add_topic("basic", "Basics", "wiki/icon_basics.png");
tut_add_topic("gates", "Gates", "wiki/icon_gates.png");
tut_add_topic("mem", "Sequential Circuits", "wiki/icon_memory.png");
tut_add_topic("math", "Math", "wiki/icon_math.png");


add_wiki("basic", "tldr", "TLDR")
add_wiki("basic", "hotkeys", "Hotkeys")
add_wiki("basic", "wires1", "Basic wires")
add_wiki("basic", "wires_crossing", "Crossing Wires")
add_wiki("basic", "wires_io", "Wires IO")
add_wiki("basic", "bit_order", "Bit Order")
add_wiki("basic", "posint", "Number Representation")
add_wiki("basic", "customlevel", "Custom Level")

add_wiki("gates", "nand", "Nand Gate")
add_wiki("gates", "not", "Not Gate")
add_wiki("gates", "and", "And Gate")
add_wiki("gates", "or", "Or Gate")
add_wiki("gates", "xor", "Xor Gate")
add_wiki("gates", "decoder1", "Decoder")
add_wiki("gates", "mux", "Mux")
add_wiki("gates", "demux", "Demux")

add_wiki("mem", "srlatch", "SR Latch")
add_wiki("mem", "dlatch", "D Latch")
add_wiki("mem", "dflipflop", "D Flip Flop")
add_wiki("mem", "synchronous", "Synchronous Circuits")
add_wiki("mem", "meminit", "Memory Initialization")
add_wiki("mem", "propdelay", "Propagation Delay")
add_wiki("mem", "setuphold", "Setup and Hold Time")

add_wiki("math", "halfadder", "Half Adder")
add_wiki("math", "fulladder", "Full Adder")
add_wiki("math", "rcadder", "Ripple Carry Adder")
add_wiki("math", "bit_shifting", "Bit Shifting")
add_wiki("math", "barrel_shifter", "Barrel Shifter")
add_wiki("math", "logarithmic_shifter", "Logarithmic Shifter")
