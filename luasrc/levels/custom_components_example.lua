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
    id='CUSTOMCOMP',
    icon = "../luasrc/imgs/levels/custom_example_level.png",
    name = 'Custom Components Example',
    unlockedBy='RAM8',
    desc = [[

!img:imgs/levels/chain_guy_image.png

Example level with custom components. No objective.

The idea is to show how the user can extend Circuit Artist to add its own levels and components to interact with the circuit (for example, if you want to add instructions for a custom CPU).

You have an example of custom (reusable) components such as:

- A WASD Keyboard input
- A ROM component
- A RAM component
- A RAM Display component

You can create your own components and levels in the `luasrc/scripts/` folder of your game installation. (if the folder is not there you can create one from the template in `luasrc/template_scripts/`)

You can look at the `luasrc/` folder for references/examples.

The game will look for the `scripts/init.lua` script at initialization, so you can import/register your levels and scripts from there.

You can open the game folder in steam via right click on the game name in "library", then go to "Manage" -> "Browse Local Files".

`Attention:` Mind not modifying scripts outside the `luasrc/scripts/` folder because they are overidden in updates.
]],
    chips = chips,
})


