--[[
Minimal example using rewind feature.

You can enable the rewind feature with the EnableRewind() function, which
should be called during _Setup().

The idea is that **no modification to "state" shall be done directly**.
Instead, you should create a "patch" object, along with two functions: a
_Forward() function that reads the patch object and does the modification
forward, and a _Backward() function that does a similar operation but
backwards: it reverts the state position.

So, instead of:

function _Update()
   var = 1
end

You'll want to do:

function _Update()
   return  {varDiff=1}
end

function _Forward(patch)
  var = var + patch.varDiff
end

function _Backward()
  var = var - patch.varDiff
end


Behind the hood, the "patch" objects are serialized and stored in a queue so
when you rewind/forward in the game it will only call the _Forward() and
_Backward() functions instead of the _Update()'s.

The patch information should contain enough information to do both the forward
and backward operations! So for example, you can't just keep the "new value" in
the patch, otherwise you won't be able to trace back in backward operation.
Instead you can store things such as the difference or a XOR value of the
variable.

Another easy way to track things in the patch object is to store triple-like
stuff like {variableId, oldValue, newValue}. The engine will store the complete
patch object by value, so you shouldn't store reference to other values there,
or it won't work / might occupy too much space. For memories for example you
should not put the whole memory in the patch, but instead only reference the
part of the memory that has changed and the changes.


Under the hood, the engine will call:

_Setup() <-- called once the script is loaded
<Player starts simulation>
_Start() <-- Called to initialize simulation variables
PATCHS = []
REPEAT:
  <Circuit is simulated by the engine>
  patch = _Update() <-- Can read/write to ports and performs logic
  PATCHS = PATCHS + [patch]
  _Forward(patch) <-- Actually Changes the state of the variables
  IF TIME TO DRAW:
     _Draw() <-- called every rendering frame

When rewind is called:
  <backwards circuit>
  _Backward(patch[N])
  <backwards circuit>
  _Backward(patch[N-1])
  <backwards circuit>
  _Backward(patch[N-2])
  <backwards circuit>
  _Backward(patch[N-3])
  Until reaches target tick.


]]--


-- Called once when the script is loaded
function _Setup()
  -- Port declaration
  PORT_CLK = AddPortOut(1, 'clk', LEFT)
  PORT_COUNTER = AddPortOut(10, 'counter', LEFT)
  PORT_INC = AddPortIn(1, 'inc', LEFT)

  -- Enabling rewind means that the _Forward(patch) and _Backward(patch)
  -- functions need to be implemented, otherwise the level will crash
  EnableRewind()
end

-- Initializes the simulation variables.
-- Called every time the simulation starts.
function _Start()
  clock = 0 -- alternates between 1 and 0 every cycle
  cycle = 0 -- counts cycles
  counter = 0 -- Incremented by "inc" every cycle
end

-- Called every cycle
function _Update()
  -- Reads the "inc" value as input.
  local inc = ReadPort(PORT_INC)
  -- Writes the values of clock
  WritePort(PORT_CLK, clock)
  -- Writes the current counter state (BEFORE the update)
  WritePort(PORT_COUNTER, counter)
  -- Now creates the patch for the circuit state to be updated
  local patch = {diff=inc}
  -- The update function must return the patch object. This object is
  -- serialized/ stored by the engine under the hood, so try not to make it too
  -- big.
  return patch
end

-- Performs the change in the state forwards
-- This replaces the explicit change of state that should be happening in _Update()
function _Forward(patch)
  clock = 1 - clock
  cycle = cycle + 1
  counter = counter + patch.diff
end

-- Performs the change in the state backwards
function _Backward(patch)
  clock = 1 - clock
  cycle = cycle - 1
  counter = counter - patch.diff
end

function _Draw()
  local white = {255, 255, 255, 255};
  rlPushMatrix()
  rlScalef(6,6,1)
  rlTranslatef(20,20,0)
  local black = {0,0,0,255}
  local red = {255,0,0,255}
  DrawText("clk=" .. clock.. " cycle=" .. cycle .. " counter=" .. counter , 1, 1, black)
  DrawText("clk=" .. clock.. " cycle=" .. cycle .. " counter=" .. counter , 0, 0, red)
  rlPopMatrix()
end

