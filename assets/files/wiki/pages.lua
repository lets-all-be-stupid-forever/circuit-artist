
function MyAddWikiItem(topic, id)
  local icon = "wiki/icons/" .. id .. ".png"
  local name = T['wiki_' .. id .. '_name']
  local desc = T['wiki_' .. id .. '_text']
  AddWikiItem(topic, id, name, desc, icon)
end


AddWikiTopic("basic", T.wiki_topic_basics, "wiki/icons/icon_basics.png");
AddWikiTopic("gates", T.wiki_topic_gates, "wiki/icons/icon_gates.png");
AddWikiTopic("mem", T.wiki_topic_mem, "wiki/icons/icon_memory.png");
AddWikiTopic("math", T.wiki_topic_math, "wiki/icons/icon_math.png");

local tldrText= [[


!img:wiki/imgs/help_small.png
]]
AddWikiItem("basic", "tldr", T.wiki_tldr_title, tldrText, "wiki/icons/tldr.png")

MyAddWikiItem("basic", "hotkeys")
MyAddWikiItem("basic", "wires1")
MyAddWikiItem("basic", "wires_crossing")
MyAddWikiItem("basic", "wires_io")
MyAddWikiItem("basic", "bit_order")
MyAddWikiItem("basic", "posint")
MyAddWikiItem("basic", "customlevel")

MyAddWikiItem("gates", "nand")
MyAddWikiItem("gates", "not")
MyAddWikiItem("gates", "and")
MyAddWikiItem("gates", "or")
MyAddWikiItem("gates", "xor")
MyAddWikiItem("gates", "decoder1")
MyAddWikiItem("gates", "mux")
MyAddWikiItem("gates", "demux")

MyAddWikiItem("mem", "srlatch")
MyAddWikiItem("mem", "dlatch")
MyAddWikiItem("mem", "dflipflop")
MyAddWikiItem("mem", "synchronous")
MyAddWikiItem("mem", "meminit")
MyAddWikiItem("mem", "propdelay")
MyAddWikiItem("mem", "setuphold")

MyAddWikiItem("math", "halfadder")
MyAddWikiItem("math", "fulladder")
MyAddWikiItem("math", "rcadder")
MyAddWikiItem("math", "bit_shifting")
MyAddWikiItem("math", "barrel_shifter")
MyAddWikiItem("math", "logarithmic_shifter")
