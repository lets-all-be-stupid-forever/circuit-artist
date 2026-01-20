

function _Setup()
  PORT_A = AddPortOut(7, 'a')
  PORT_B = AddPortIn(7, 'b')
end


function _Start()
  print('started')
end

function _Update()
  local n = ReadPort(PORT_B)
  WritePort(PORT_A, (n + 1) % 128)
end

function _Draw()
  local white = {255, 255, 255, 255};
  rlPushMatrix()
  rlScalef(3,3,1)
  DrawRectangle(0,0,200,500,{255,255,255,150})
  DrawText("Hello!", 0, 0, {0,0,0,255})
  rlPopMatrix()
end
