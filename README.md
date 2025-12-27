![logo](assets/logo3.png)

Circuit Artist is a digital logic circuit drawing game.

## Changes in 1.1

I've reworked the simulation engine to use a variable-delay event-driven simulation taking into account the topology of the wires to create a distance / propagation delay map based on elmore delay calculation for trees. It also takes into account fanout nands, so you have things like the higher the fanout the higher the delay, trees propagating differently then "lines" etc, to a have a more "accurate" simulation so players can play/develop intuition for real circuit design (sort of).

There's also a simplified energy calculation formula/metric to explore circuit energy efficiency vs speed (even though its still not fully developed yet in the game).

Now clocked components won't work for free, need to make them efficient, adding an extra challenge/fun dimension to the game.

The distances are mapped to pixels so players can visualize the wires "propagating" with glow, which I find cool and helps understand how things work. Rendering is done in real time with some shaders.

I've also added a delta-based design on the simulation to allow simulation to be paused and controlled back and forth so players can interact with it. Particularly useful for debugging "cyclic" circuits.

Players can also add layers as in photoshop, (up to 3 layers), and layer wires can connect with neighboor layers, although NANDs are only allowed on the bottom layer. Wires on upper layers have a higher "propagation" speed.

Also added a lower-pace campaign system so new users can solve problems little by little and learn concepts gradually, testing/refining solutions and tracking progress. It's still on early days, plan to put more stuff later.

Lately I've added an inventory-like UI for placing blueprints so players can build it's library and re-use it, which also adds a new progression dimension to the game.

The simulation also is no longer "immediate", players can build their own clocks on sandbox mode by just exploring the delay mechanism.

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
