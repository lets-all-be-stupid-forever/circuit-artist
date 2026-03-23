
function MyAddWikiItem(topic, id)
  local icon = "wiki/icons/" .. id .. ".png"
  local name = T['wiki_' .. id .. '_name']
  local desc = T['wiki_' .. id .. '_text']
  AddWikiItem({
    topic=topic,
    id=id,
    name=name,
    desc=desc,
    icon=icon,
  })
end


AddWikiTopic({
  id="basic",
  name=T.wiki_topic_basics,
  icon="wiki/icons/icon_basics.png",
});

AddWikiTopic({id="gates", name=T.wiki_topic_gates, icon="wiki/icons/icon_gates.png"});
AddWikiTopic({id="mem",   name=T.wiki_topic_mem,   icon="wiki/icons/icon_memory.png"});
AddWikiTopic({id="math",  name=T.wiki_topic_math,  icon="wiki/icons/icon_math.png"});

local tldrText= [[


!img:wiki/imgs/help_small.png
]]
AddWikiItem({topic="basic", id="tldr", name=T.wiki_tldr_title, desc=tldrText, icon="wiki/icons/tldr.png"})

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
