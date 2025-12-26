Credits
-------

Game Framework
raylib
--> The raylib version has been modified a bit, i've added a "multi-texture output" support in rlgl.h file.
I needed to do that for a workaround around computing shaders. It seems
computing shaders needed a version of OpenGL that was not supported in other
systems like MacOS, so I opted to add this modif there instead. The code is in the third_party/raylib folder.
raylib.com

C data structures
stb_ds.h
https://github.com/nothings/stb/blob/master/stb_ds.h

Font
m5x7 and m3x6 by Daniel Linssen.
https://managore.itch.io/m5x7

File Dialog
Native File Dialog by Michael Labbe
https://github.com/mlabbe/nativefiledialog

Copy-paste
Clip library by David Capello / Aseprite.
https://github.com/aseprite/clip

JSON in C
JSON-C - A JSON implementation in C
https://github.com/json-c/json-c

Lua 5.4
https://www.lua.org/

Lua Serialization
msgpack-c
https://github.com/msgpack/msgpack-c  (c_master branch)
