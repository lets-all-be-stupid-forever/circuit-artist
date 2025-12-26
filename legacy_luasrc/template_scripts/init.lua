--[[
This is just a template file, in order for it to work you'll need to copy-paste
it to the luasrc/scripts/ folder. The scripts/ folder is never changed by steam
on updates on uninstalls.

The app will automatically look for the 'scripts/init.lua' folder at startup
and run it. That's where you call/create your levels. Don't hesitate looking at
other scripts for reference, or post something at github if you have doubts.

When developping locally, you might want to change the steam launcher to the
console launcher, so you have access to the logs and error messages.

Even better, you might want to manually launch the application from the
terminal. Just go to the installation folder's bin/ and launch it as
./ca_console.exe, this way the console messages will be in your terminal (you
might even integrate in your development workflow) (You need to launch it from
the bin/ folder, or set the CHDIR to the bin/ folder, because there are many
relative paths from there)

Mind that the app loads the script at startup, so you'll need to relaunch at
every modification (unless you do some adaptions at lua side to support reload).

Import your scripts from here.

--]]

-- Imports the custom script
require 'scripts.example_custom_level'

-- Makes the custom script be the initially selected one
-- The name string should match the name defined in the level definition.
setInitialLevelByName('My Level')

-- You can use this function to modify the initial image so you don't need to
-- reload your image everytime you relaunch the application/change your script.
-- Path needs to be relative to the bin/ folder or an absolute path!
setStartupImage('../luasrc/template_scripts/my_startup_image.png')
