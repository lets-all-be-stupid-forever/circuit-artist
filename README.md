![logo](assets/logo3.png)

Circuit Artist is a digital logic circuit drawing game.

Features:

- Layers (up to 3 layers).
- Propagation visualization.
- Simulation rewind / time control
- Distance-based propagation.
- Inventory-like blueprints
- Campaign / Levels `(Campaign still in construction)`
- Circuit activity has audio feedback

## The Simulation Engine

- Variable-delay event-driven simulation.
- Uses an elmore-distance approximation of circuit delay and nand activation. High fanout? Slower delay. Long Wires? Squared delay penalty. Event
- NAND only allowed on bottom layer.
- Top layers "propagate" faster.
- Reversible: Can pause time and rewind/forward simulation.
- Realtime visualization of wire states and propagation.
- Realtime interaction with any wires
- "Energy" calculation (although not fully exposed/explored in the UI yet)

## Links

- [Steam](https://store.steampowered.com/app/3139580/Circuit_Artist/) (Support us ;) )
- [Discord](https://discord.gg/McpSTEW5jU) (Please share your feedback!)
- [Blog](https://circuitartistgame.com/)

## Gifs

![Image](https://github.com/user-attachments/assets/b5a1b21d-5920-4d14-b2af-e8450a96fb24)

![Image](https://github.com/user-attachments/assets/5b655739-0cfd-4bb1-b73f-0f7315160bc0)

![Image](https://github.com/user-attachments/assets/361a9a42-3f4a-4043-bb19-6f13ae5758ab)

![Image](https://github.com/user-attachments/assets/0bb1a7d2-9ee1-4059-b794-561d2809948f)

![Image](https://github.com/user-attachments/assets/ca215187-4ad6-4618-9700-dfa31cf6481c)

![Image](https://github.com/user-attachments/assets/24ed1334-3a3a-40c5-b3f7-823d9a124eef)

## Game Rules

- Little pixel triangles are NANDs.
- Black pixels are background.
- Everything else is a wire.

### Clone, compile, run on Linux/MacOS

```
git clone https://github.com/lets-all-be-stupid-forever/circuit-artist.git
cd circuit-artist/
git submodule init
git submodule update
mkdir build
cd build/
cmake ..
make
```

Made with [raylib](https://www.raylib.com/).

## License

GPLv3. See LICENSE file. For dependencies, see `third_party` folder.
