
local Sandbox = Component:extend()

function Sandbox:setup()
  self.ports = {
    {name="power_on_reset", width=1, input=false},
    {name="clock", width=1, input=false},
  }
end

function Sandbox:start()
  self.v_cycle = var(-1)
end

function Sandbox:update()
  local cycle = self.v_cycle() + 1

  -- -1)->0->1->2->3 -> 4 ->5
  --    l  h  l  h   l     h   (low/high clock)
  -- rise 1      2
  -- POR stays on for 2 rising edges before falling.
  -- 3rd rising edge has por=0
  -- por needs to change at the falling edge
  if cycle < 4 then
    pset(0, 1)
  elseif cycle == 4 then
    pset(0, 0)
  end
  local clk = cycle % 2
  pset(1, clk)
  self.v_cycle(cycle)
end


add_chip_instance(Sandbox())
