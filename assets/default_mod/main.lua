function MyAddWikiItem(topic, id)
  local icon = "wiki/icons/" .. id .. ".png"
  local name = TR('wiki_' .. id .. '_name')
  local desc = TR('wiki_' .. id .. '_text')
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
  name=TR('wiki_topic_basics'),
  icon="wiki/icons/icon_basics.png",
});

AddWikiTopic({id="gates", name=TR('wiki_topic_gates'), icon="wiki/icons/icon_gates.png"});
AddWikiTopic({id="mem",   name=TR('wiki_topic_mem'),   icon="wiki/icons/icon_memory.png"});
AddWikiTopic({id="math",  name=TR('wiki_topic_math'),  icon="wiki/icons/icon_math.png"});

local tldrText= [[


!img:wiki/imgs/help_small.png
]]
AddWikiItem({topic="basic", id="tldr", name=TR('wiki_tldr_title'), desc=tldrText, icon="wiki/icons/tldr.png"})

MyAddWikiItem("basic", "hotkeys")
MyAddWikiItem("basic", "wires1")
MyAddWikiItem("basic", "wires_crossing")
MyAddWikiItem("basic", "wires_io")
MyAddWikiItem("basic", "posint")
MyAddWikiItem("basic", "significant")
MyAddWikiItem("basic", "bit_order")
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
MyAddWikiItem("math", "signed")
MyAddWikiItem("math", "comparator")
MyAddWikiItem("math", "bit_shifting")
MyAddWikiItem("math", "barrel_shifter")
MyAddWikiItem("math", "logarithmic_shifter")


function AddCampaignLevel(ctx)
  AddLevel({
    id=ctx.id,
    group=ctx.group,
    name=TR(ctx.id .. '_name'),
    deps=ctx.deps,
    description=TR(ctx.id .. '_desc'),
    kernel="levels/kernels/" .. ctx.id .. ".lua",
    extra_text=ctx.extra_text,
  })
end

function RegisterTxt(gid, groupTxt, groupDeps)
  AddGroup({
    id=gid,
    icon='levels/icons/campaign_' .. gid .. '.png',
    name=TR(gid .. '_name'),
    desc=TR(gid .. '_desc'),
    deps=groupDeps,
  })
  local prev = nil
  for line in groupTxt:gmatch("[^\n]+") do
    line = line:match("^%s*(.-)%s*$")
    if line ~= "" then
      local tokens = {}
      for token in line:gmatch("%S+") do
        table.insert(tokens, token)
      end
      local et = {}
      for i = 2, #tokens do
        local key, value = tokens[i]:match("^(%w+)=(.+)$")
        if key == "wiki" then
          table.insert(et, {wiki=value})
        elseif key == "text" then
          table.insert(et, {title=TR(value .. '_title'), text=TR(value .. '_text')})
        elseif key == "img" then
          local img_name = value:match("^([^,]+)")
          local s_val = value:match(",s=(%d+)")
          table.insert(et, {title=TR(img_name .. '_title'), img='levels/imgs/' .. img_name .. '.png', scale=tonumber(s_val)})
        end
      end
      local deps = prev and {prev} or nil
      AddCampaignLevel({id=tokens[1], group=gid, deps=deps, extra_text=et})
      prev = tokens[1]
    end
  end
end

RegisterTxt('basics1', [[
wires1   wiki=wires1
wires2   wiki=wires_crossing
wires3   wiki=wires_io
wires4   wiki=bit_order text=wires4_tip text=wires4_pin
nand     wiki=nand img=nand_example,s=8 img=nand_schema,s=4
not      wiki=not img=not_picture,s=4 img=not_schema,s=4
and      wiki=and img=and_picture,s=4 img=and_schema,s=4
or       wiki=or img=or_picture,s=4 img=or_schema,s=4
xor      wiki=xor img=xor_picture,s=4 img=xor_schema,s=4
a_eq_b1  img=a_eq_b1_tip,s=4
a_eq_b2  text=a_eq_b2_tip
]], nil)

RegisterTxt('sevseg1', [[
match7 wiki=posint
match7_or_23
decoder1 wiki=decoder1
decoder2
decoder_2bit
decoder3
match_many text=match_many_tip text=match_many_table
sevenseg img=sevenseg1,s=4
]], {'basics1'})

RegisterTxt('routing1', [[
mux_2_1 wiki=mux img=mux1,s=4 img=mux2,s=4 img=mux3,s=4
mux_4_1 text=mux_4_1_larger img=mux4,s=4
mux_4_2
bus2 text=bus2_example img=bus_example,s=4
demux_1_2 wiki=demux  text=demux_1_2_analogy text=demux_1_2_analogy2 img=demux1,s=4 img=demux2,s=4 img=demux3,s=4
demux_1_4 img=demux4,s=4
demux_2_4
router text=router_example img=router_example2,s=4
]], {'sevseg1'})

RegisterTxt('memory1', [[
latch_door            wiki=srlatch text=latch_door_think
dlatch                wiki=dlatch text=dlatch_example img=mem2,s=4 img=mem3,s=4 img=mem4,s=4
photo                 wiki=dflipflop text=photo_analogy
combo_detector        wiki=propdelay wiki=setuphold text=combo_detector_clock text=combo_detector_timing text=combo_detector_delay text=combo_detector_history text=combo_detector_output
dflipflop_with_enable wiki=dflipflop text=dflipflop_with_enable_hint text=dflipflop_with_enable_example text=dflipflop_with_enable_tests text=dflipflop_with_enable_comment img=D_with_enable,s=4
dff_w_r               text=dff_w_r_hint text=dff_w_r_comment img=D_with_reset,s=4 text=dff_w_r_tests
register4             text=register4_hint text=register4_tests
registerfile          text=registerfile_hint text=registerfile_tests
npu1                  wiki=synchronous text=npu1_data text=npu1_asm text=npu1_prog
]], {'routing1'})

RegisterTxt('aplusb', [[
halfadder wiki=halfadder
fulladder wiki=fulladder
adder4bit wiki=rcadder
nega wiki=signed
subtractor text=subtip
comparator1 wiki=comparator
comparator2 wiki=comparator
comparator3 text=compnote
amul2 text=mul2tip
amul3 text=mul3tip
amulb
]], {'basics1'})

RegisterTxt('shifter', [[
shift1 wiki=bit_shifting text=logic_right_shift
shift2 text=rotate_shift
ashift1 text=arithmetic_right_shift
barrel1 wiki=barrel_shifter
barrel2
abarrel1
lbarrel1
clbarrel1
funnel_shifter img=funnel1,s=4
barrel32
]], {'aplusb', 'routing1'})

