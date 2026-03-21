print('Inside global lua!')

-- Putting level here

print('-------------------')
print('-- LUA LOADED -----')
print('-------------------')

--[[
function addcomb(id, name, group, deps, assets)
  if group == nil then
    group = 'custom'
  end
  if assets == nil then
    assets = {}
  end
  add_level({
    id=id,
    group=group,
    name=name,
    assets=assets,
    deps=deps,
    icon="levels/icons/" .. id .. ".png",
    description=loadtxt('levels/desc/' .. id .. '.txt'),
    content={
      "shared/classic.lua",
      "shared/preload_chip.lua",
      "shared/comb_level.lua",
      "levels/kernel/" .. id .. ".lua",
    }
  })
end
]]--

function easy_add_level2(ctx)
  if ctx.group == nil then
    ctx.group = 'custom'
  end
  if ctx.assets == nil then
    ctx.assets = {}
  end
  if ctx.content == nil then
    ctx.content = {
      "shared/classic.lua",
      "shared/preload_chip.lua",
      "shared/comb_level.lua",
      "levels/kernel/" .. ctx.id .. ".lua",
    }
  end

-- loadtxt('levels/desc/' .. ctx.id .. '.txt'),
  add_level({
    id=ctx.id,
    group=ctx.group,
    name=T[ctx.id .. '_name'],
    assets=ctx.assets,
    deps=ctx.deps,
    icon="levels/icons/" .. ctx.id .. ".png",
    description=T[ctx.id .. '_desc'],
    content=ctx.content,
    extra_text=ctx.extra_text,
  })
end

function easy_add_level(ctx)
  if ctx.group == nil then
    ctx.group = 'custom'
  end
  if ctx.assets == nil then
    ctx.assets = {}
  end
  if ctx.content == nil then
    ctx.content = {
      "shared/classic.lua",
      "shared/preload_chip.lua",
      "shared/comb_level.lua",
      "levels/kernel/" .. ctx.id .. ".lua",
    }
  end

-- loadtxt('levels/desc/' .. ctx.id .. '.txt'),
  add_level({
    id=ctx.id,
    group=ctx.group,
    name=ctx.name,
    assets=ctx.assets,
    deps=ctx.deps,
    icon="levels/icons/" .. ctx.id .. ".png",
    description=ctx.description,
    content=ctx.content,
    extra_text=ctx.extra_text,
  })
end

----------------------------------------------------
Import 'en.lua'
Import 'custom_campaign.lua'
Import 'basics_campaign.lua'
Import 'sevseg_campaign.lua'
Import 'routing_campaign.lua'
Import 'memory_campaign.lua'


-- easy_add_level({
--     id="doorpwd",
--     name="Door Lock With Password",
--     group='memory1',
--     deps={},
--     description=[[
--
-- Create a door lock system with password. Instead of a unlock button, the door has now 3 buttons:
--
-- - A `0` button.
-- - A `1` button.
-- - A `lock` button.
--
-- At any time the door can be locked or unlocked. The door password is known beforehand: 00101101, which means that once the user enters the code in this exact sequence the door should unlock and remain unlocked, until he presses again the "lock button", which also resets the previous entered code values.
--
-- So:
--
-- - When user enters 0 -> 0 -> 1 -> 0 -> 1 -> 1 -> 0 -> 1, the door should unlock. (and remain unlocked unless the user presses the "lock" button)
-- - When user presses "lock", the door should unlock and the system works as if the user had previously typed `0` 100 times (ie the "history" becomes all 0).
--
-- The tests start with the user locking the door (ie pressing the lock button).
--
-- Again, here the user can only press one button at a time, and the user presses the buttons significantly slower between one press and the next (ie, there won't be "pressing too fast" scenarios).
--
-- PS: It's ok for it to lock/unlock only when user releasees the button.
--
-- ]],
-- extra_text= {
--   {
--     title="Tip",
--     text=[[
-- You'll need to introduce a small time delay between inputs and your clock signal: the clock signal must arrive after the input signals.
-- ]]
--   }
-- }
-- })



-- !img:imgs/tutorial/mux1.png
--
-- A `MUX` is a like a bit `selector`: Given 2 input bits `a` and `b`, and a selection bit `s`, if `s` is 0, `MUX` returns the value of `a`, and if `s` is 1, `MUX` returns the value of `b`.
-- !img:imgs/tutorial/mux3.png
-- Muxes work like selectors (or a "switch"). They're useful for example when you have multiple calculations and want to select only one of the results. It's like an "if" statement in programming: IF S=0, RETURN A; ELSE IF S=1 RETURN B;.
-- !img:imgs/tutorial/mux2.png
-- A mux can be implemented as shown in the diagram above.
-- You can also create muxes with more inputs by combining muxes having fewer inputs:
-- !img:imgs/tutorial/mux4.png


-- basics2 = 'basics2'
-- add_group({name="Decoders", id=basics2})
-- addcomb("decoder1", "Decoder", basics2)
-- addcomb("demux", "Demux", basics2, {'decoder1'})
-- addcomb("decoder2", "Decoder With Enable", basics2, {'decoder1'})
-- addcomb("decoder3", "5:32 Decoder", basics2, {'decoder2'})
--
-- math1 = 'math1'
-- add_group({name="Math I", id=math1})
-- addcomb("halfadder", "Half Adder", math1)
-- addcomb("fulladder", "Full Adder", math1, {'halfadder'})
-- addcomb("adder4bit", "4 bits Adder", math1, {'fulladder'})
-- addcomb("subtractor", "A - B", math1, {'adder4bit'})
--
-- comparing = 'comparing'
-- add_group({name="Bits", id=comparing})
-- addcomb("a_eq_b1", "A=B I", comparing)
-- addcomb("a_eq_b2", "A=B II", comparing, {'a_eq_b1'})
-- addcomb("comparator1", "Comparator", comparing, {'a_eq_b1'})
-- addcomb("comparator2", "4 Bits Comparator", comparing,  {'comparator1'})
-- addcomb("comparator3", "4 Bits Comparator (Signed)", comparing, {'comparator2'})
--
--
-- shifting = 'shifting'
-- add_group({name="Shifting", id=shifting})
-- addcomb("shift1", "Bit Shifter", shifting )
-- addcomb("shift2", "Cyclic Bit Shifter", shifting,  {'shift1'})
-- addcomb("barrel1", "Bit Shifter", shifting, {'shift2'})
-- addcomb("barrel2", "Cyclic Shifter", shifting, {'barrel1'})


----------------------------------------------------
-- addcomb("mof3", "Multiple of 3")
-- addcomb("ram", "Basic Memory")
--
-- local sevenSegAssets = {
--   'levels/sevenseg/seven_seg_display.png',
-- }
-- addcomb("sevenseg", "7-Segment Display", nil, nil, sevenSegAssets)

tut_add_topic("basic", "Basics", "wiki/icon_basics.png");
tut_add_topic("gates", "Gates", "wiki/icon_gates.png");
tut_add_topic("mem", "Sequential Circuits", "wiki/icon_memory.png");
tut_add_topic("math", "Math", "wiki/icon_math.png");

-- function add_wiki(topic, id, name)
--   tut_add_item(topic, id, name, loadtxt("wiki/" .. id .. "/" .. id .. ".txt"), "wiki/" .. id .. "/icon_" .. id ..".png");
-- end

function add_wiki(topic, id, name)
  local desc = "wiki/desc/" .. id .. ".txt"
  local icon = "wiki/icons/" .. id .. ".png"
  tut_add_item(topic, id, name, loadtxt(desc), icon)
end


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

-- tut_add_item("basic", "wires1", "Basic wires", loadtxt("wiki/wires1/wires1.txt"), "wiki/wires1/icon_wires1.png");
-- tut_add_item("basic", "nand", "Nand Gates",    loadtxt("wiki/nand/nand.txt"), "wiki/basic/nand/icon_nand.png");
