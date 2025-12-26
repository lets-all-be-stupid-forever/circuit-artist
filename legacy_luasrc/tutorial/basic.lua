return {
  name= "Basics",
  text = [[

  Each non-black pixel in the image represents either a wire or a NAND logic gate.

  You can load arbitrary images in the game, either by dragging, opening or copy-pasting. However, mind that only completely black pixels are considered background, all other pixels will be considered wires or NANDs.

  You can use the game in `sandbox` mode or try to solve `challenges`. The button in the upper-right corner of the screen allows you to switch modes.

  You can also create your own "challenge", or extend the sandbox mode via lua script. You can access the game installation folder, and on the `luasrc/` folder you will find most scripts of the game which you can play with. You can use it to create new challenges, write a "test" for a circuit you want to create, etc (just remember keeping a backup in case of automatic updates as API is not stable yet). The source code for all existing challenges are present there.

  You could use it for instance for creating your own CPU and write machine code as lua text. You could also reproduce your favorite youtube's tutorial, etc.

  You can change the color palette in the lua code.

  ]]
}
