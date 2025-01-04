local LevelComponent = require 'level_component'
local Clock = require 'clock'

local BasicKeyboard = require 'component_examples.basic_keyboard'
local BasicROM = require 'component_examples.basic_rom'
local BasicRAM = require 'component_examples.basic_ram'
local BasicRAMDisplay = require 'component_examples.basic_ram_display'

local chips = {}
table.insert(chips, Clock(false))
table.insert(chips, BasicKeyboard())
table.insert(chips, BasicROM())

-- Here's an example on how to share data between components:
-- Our "display" component directly access the memory from the RAM component,
-- that doesnt even know the display exists. In this specific case it assumes
-- the table "memory" is created in the constructor of the ram component, but
-- a callback could also be used.
-- Mind also that you can have components that dont directly interact with the
-- image chip (the ramdisplay here has an "display_offset" pin input, but it
-- could have no pin and it would still work)
local ram = BasicRAM()
local ramDisplay = BasicRAMDisplay(ram.memory, 16, 16)
table.insert(chips, ram)
table.insert(chips, ramDisplay)


addLevel({
    icon = "../luasrc/imgs/levels/custom_icon.png",
    name = 'Custom Components',
    desc = [[

!img:imgs/levels/custom_icon.png

Advanced Sandbox mode. No objective.

You have an example of custom (reusable) components such as:

1. A WASD Keyboard input (`key_w` wire will activate when you type the W keyboard key, same for `key_a`, ` key_s` and `key_d`)

2. A ROM component (`rom_ra` is read address, `rom_rd` is the read value)

3. A RAM component (`ram_wa` is the write address, `ram_wd` is the write data, `ram_we` is the write enable (we=1 means write, we=0 means read) and `ram_rd` is the read address)

4. A RAM Display component (will automatically display the `ram` content in the top-right of the screen. The content can be shifted by an offset, to simulate offscreen buffers). This is an example on how to communicate/interact between components.

Check the `luasrc/levels/custom_components.lua` script for more details.

You can create your own components and levels in the `luasrc/scripts/` folder of your game installation. (if the folder is not there you can create one from the template in `luasrc/template_scripts/`)

You can look at the `luasrc/` folder for references/examples.

The game will look for the `scripts/init.lua` script at initialization, so you can import/register your levels and scripts from there.

You can open the game folder in steam via right click on the game name in "library", then go to "Manage" -> "Browse Local Files".

`Attention:` Mind not modifying scripts outside the `luasrc/scripts/` folder because they are overidden in updates.
]],
    chips = chips,
})
