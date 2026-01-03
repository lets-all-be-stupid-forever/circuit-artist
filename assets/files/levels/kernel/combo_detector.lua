-- Button Combo Detector
-- Detects if the last 4 button presses were all button A

math.randomseed(42)

local function makeCases()
  local cases = {}

  -- History of last 4 presses (1=A, 0=B)
  local history = {0, 0, 0, 0}
  local press_count = 0

  -- Initial sequence testing specific patterns
  local press_sequence = {
    'A', 'A', 'A', 'A',  -- First 4 presses (output becomes valid after 4th)
    'B',                  -- Break the streak
    'A', 'A', 'A', 'A',  -- Build up again
    'A',                  -- Still all A's in last 4
    'B',                  -- Break
    'B',                  -- Still broken
    'A', 'A', 'A',       -- Building...
    'B',                  -- Break at 3
    'A', 'A', 'A', 'A',  -- Full streak again
    'B',                  -- Break
    'A', 'B', 'A', 'B',  -- Alternating
    'A', 'A', 'A', 'A',  -- Full streak
  }

  -- Add 100 random presses
  for i = 1, 100 do
    if math.random() < 0.6 then
      table.insert(press_sequence, 'A')
    else
      table.insert(press_sequence, 'B')
    end
  end

  for i, press in ipairs(press_sequence) do
    local button_A = (press == 'A') and 1 or 0
    local button_B = (press == 'B') and 1 or 0

    -- Shift history and add new press
    table.remove(history, 1)
    table.insert(history, button_A)
    press_count = press_count + 1

    -- Calculate expected output
    local expected = nil
    if press_count <= 4 then
      expected = nil  -- Don't care for first 4 presses
    else
      -- Check if all 4 in history are 1 (all A presses)
      if history[1] == 1 and history[2] == 1 and history[3] == 1 and history[4] == 1 then
        expected = 1
      else
        expected = 0
      end
    end

    -- Button pressed (clock rising edge)
    table.insert(cases, {
      button_A = button_A,
      button_B = button_B,
      combo_detected = expected,
      name = "Press " .. i .. ": " .. press .. " -> " .. (expected == nil and "n/a" or tostring(expected))
    })

    -- Button released (clock low)
    table.insert(cases, {
      button_A = 0,
      button_B = 0,
      combo_detected = expected,
      name = "Release " .. i
    })
  end

  return cases
end

easyAddTest({
  cases = makeCases(),
  ports = {
    {name="button_A", width=1, input=false},
    {name="button_B", width=1, input=false},
    {name="combo_detected", width=1, input=true},
  }
})
