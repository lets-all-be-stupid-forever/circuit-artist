![logo](assets/logo2.png)

Circuit Artist is a digital circuit drawing and simulation game. Circuits are images. You can play in sandbox mode or solve puzzles. You can use lua to interact with your circuit.

- [steam](https://store.steampowered.com/app/3139580/Circuit_Artist/)
- [discord](https://discord.gg/McpSTEW5jU)

Made with [raylib](https://www.raylib.com/).

## Game Rules

- Little pixel triangles are NANDs.
- Black pixels are background.
- Everything else is a wire.

## Screenshots

![screenshot1](assets/screenshot1.png)

![screenshot2](assets/screenshot2.png)

![screenshot3](assets/screenshot3.png)

![screenshot4](assets/screenshot4.png)

## Building

Compiles in Windows.

- Need to build within the `build/` directory (or something within the root directory so it can access `luasrc/` and `asset/` folder)
- Need to download and build [luajit](https://luajit.org/download.html) locally in the folder `LuaJIT`, so that `LuaJIT/src/`, `LuaJIT/src/lua51.dll` and `LuaJIT/src/luajit.lib` are there.

## License

GPLv3. See LICENSE file. For dependencies, see `third_party` folder.
