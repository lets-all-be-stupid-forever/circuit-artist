local Tester = require 'tester'
local math = require 'math'
local Clock = require 'clock'
local BroadcastBus = Tester:extend()

function BroadcastBus:new()
  BroadcastBus.super.new(self)
  self.has_submit = false
  self.pins = {
    {'output', 3, 'origin'},
    {'output', 8, 'C0_in'},
    {'input', 8, 'C0_out'},
    {'output', 8, 'C1_in'},
    {'input', 8, 'C1_out'},
    {'output', 8, 'C2_in'},
    {'input', 8, 'C2_out'},
    {'output', 8, 'C3_in'},
    {'input', 8, 'C3_out'},
    {'output', 8, 'C4_in'},
    {'input', 8, 'C4_out'},
    {'output', 8, 'C5_in'},
    {'input', 8, 'C5_out'},
    {'output', 8, 'C6_in'},
    {'input', 8, 'C6_out'},
    {'output', 8, 'C7_in'},
    {'input', 8, 'C7_out'},
  }
  local schedule = {}

  for i=1,32 do
    local inputs = {}
    for j = 1,8 do
      inputs[j] = math.random(256) - 1
    end
    local src = math.random(8) - 1
    local val = inputs[src+1]
    local s = {origin=src}
    for j = 1, 8 do
      s['C' .. j-1 .. '_out'] = val
      s['C' .. j-1 .. '_in'] = inputs[j]
    end
    table.insert(schedule, s)
  end
  self.schedule = schedule
end

addLevel({
  icon="../luasrc/imgs/levels/bus_icon.png",
  name="Broadcast Bus",
  desc=[[

!img:imgs/levels/bus2.png

You have 8 different components, each sending (`CX_in`) and receiving (`CX_out`) a 8-bit message at the same time.

The objective is to send the message from one of those components to all the others. The source component is defined by the `origin` input number ranging from 0 to 7.

`Example:`

For the inputs:

origin=3
C0_in=00
C1_in=11
C2_in=22
C3_in=33
C4_in=44
C5_in=55
C6_in=66
C7_in=77

You should have as outputs:

C0_out=33
C1_out=33
C2_out=33
C3_out=33
C4_out=33
C5_out=33
C6_out=33
C7_out=33

During test, each component will send random messages and origins. Your task is to redirect them as described above.

There will be a total of 32 tests.

    ]],
    chips = {
      Clock(true),
      BroadcastBus(4),
    },
    id='BUS',
})
