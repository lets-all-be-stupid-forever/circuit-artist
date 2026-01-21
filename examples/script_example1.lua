--[[
Example where it reads port B and writes its value +1 in port A.

This example shows how to read from a port and how to draw stuff.
Color is represented by {red,green,blue,alpha} list, with each component
ranging from 0 to 255.
]]

function _Setup()
  PORT_A = AddPortOut(7, 'a')
  PORT_B = AddPortIn(7, 'b')
end


function _Start()
  -- print's will go to terminal's output.
  -- On windows, you can run the gamme with "console" launch option
  print('started')
end

function _Update()
  local n = ReadPort(PORT_B)
  WritePort(PORT_A, (n + 1) % 128)
end

-- Draws a simple square on screen with scaling, with the text "hello"
function _Draw()
  local white = {255, 255, 255, 255};
  rlPushMatrix()
  rlScalef(3,3,1)
  DrawRectangle(0,0,200,500,{255,255,255,150})
  DrawText("Hello!", 0, 0, {0,0,0,255})
  rlPopMatrix()
end
