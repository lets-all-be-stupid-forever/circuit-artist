
function add_wiki2(topic, id)
  local icon = "wiki/icons/" .. id .. ".png"
  local name = T['wiki_' .. id .. '_name']
  local desc = T['wiki_' .. id .. '_text']
  tut_add_item(topic, id, name, desc, icon)
end


tut_add_topic("basic", T.wiki_topic_basics, "wiki/icons/icon_basics.png");
tut_add_topic("gates", T.wiki_topic_gates, "wiki/icons/icon_gates.png");
tut_add_topic("mem", T.wiki_topic_mem, "wiki/icons/icon_memory.png");
tut_add_topic("math", T.wiki_topic_math, "wiki/icons/icon_math.png");

local tldr_txt = [[


!img:wiki/imgs/help_small.png
]]
tut_add_item("basic", "tldr", T.wiki_tldr_title, tldr_txt, "wiki/icons/tldr.png")

add_wiki2("basic", "hotkeys")
add_wiki2("basic", "wires1")
add_wiki2("basic", "wires_crossing")
add_wiki2("basic", "wires_io")
add_wiki2("basic", "bit_order")
add_wiki2("basic", "posint")
add_wiki2("basic", "customlevel")

add_wiki2("gates", "nand")
add_wiki2("gates", "not")
add_wiki2("gates", "and")
add_wiki2("gates", "or")
add_wiki2("gates", "xor")
add_wiki2("gates", "decoder1")
add_wiki2("gates", "mux")
add_wiki2("gates", "demux")

add_wiki2("mem", "srlatch")
add_wiki2("mem", "dlatch")
add_wiki2("mem", "dflipflop")
add_wiki2("mem", "synchronous")
add_wiki2("mem", "meminit")
add_wiki2("mem", "propdelay")
add_wiki2("mem", "setuphold")

add_wiki2("math", "halfadder")
add_wiki2("math", "fulladder")
add_wiki2("math", "rcadder")
add_wiki2("math", "bit_shifting")
add_wiki2("math", "barrel_shifter")
add_wiki2("math", "logarithmic_shifter")
