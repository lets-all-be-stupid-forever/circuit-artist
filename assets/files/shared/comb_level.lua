local CombinatorialTest = Component:extend()
local GREEN = {0, 255, 0, 255}
local RED = {255, 0, 0, 255}
local BLACK = {0, 0, 0, 255}
local BLUE = {0, 128, 255, 255}
local WHITE = {248, 255, 203, 255}
local WHITE_ = {255, 255, 255, 255}

function bits(s)
  if s[1] == '2' then
    return nil
  else
    return tonumber(s, 2)
  end
end

function CombinatorialTest:start()
  self.v_icase = var(0)
  self.v_done = var(false)
  self.v_err = var(false)
  self.v_errors = var(nil)
end

function CombinatorialTest:update()
  local err = self.v_err()
  local done = self.v_done()
  if err or done then
    return
  end
  local icase = self.v_icase()
  if icase > 0 then
    -- First verify the previous result
    for iport=1,#self.ports do
      local port = self.ports[iport]
      if port.input then
        -- Testing
        local expect = self.cases[icase][port.name]
        local output = pget(iport-1)
        if expect ~= nil and expect ~= output then
          self.v_err(true)
          self.v_errors(
            {
              port=port.name,
              expected=expect,
              output=output,
            })
          return
        end
      end
    end
    if icase == #self.cases then
      self.v_done(true)
      notify_level_complete()
      return
    end
  end
  -- Now dispatches result of next case
  for iport=1,#self.ports do
    local port = self.ports[iport]
    if not port.input then
      pset(iport-1, self.cases[icase+1][port.name])
    end
  end
  self.v_icase(icase + 1)
  return
end


-- Draws on the screen
function CombinatorialTest:draw()
  local err = self.v_err()
  local done = self.v_done()
  local icase = self.v_icase()
  local errors = self.v_errors()

  local msgs = {}
  -- First the general status message
  if done then
    table.insert(msgs, {text='Level Complete', color=GREEN})
  elseif err then
    table.insert(msgs, {text='Failure', color=RED})
  else
    table.insert(msgs, {text='Running...', color=BLUE})
  end
  if errors ~= nil then
    table.insert(msgs, {text='Expected:', color=RED})
    table.insert(msgs, {text=errors.port .. '=' .. errors.expected})
    table.insert(msgs, {text='Got:', color=RED})
    table.insert(msgs, {text=errors.port .. '=' .. errors.output})
  end

  -- Then is the current test index
  if not done and icase > 0 then
    local numCases = #self.cases
    table.insert(msgs, {text='Test ' .. icase .. '/' .. numCases})
    table.insert(msgs, {text=self.cases[icase].name})
  end

  rl_push_matrix();
  rl_scalef(3,3,1);
  for i=1,#msgs do
    local txt = msgs[i].text
    local color = msgs[i].color or WHITE
    local lh = 8
    local pady = 1
    local bh = lh + 2*pady
    local y = (i - 1) * bh + pady
    -- local size = MeasureText(txt, lh)
    if txt ~= nil then
      local bg = BLACK
      local w = measure_text_size(txt)
      local off = 4
      draw_rectangle_pro(0, y - pady, w + 6, bh,
      0,0 ,0,
      0,0,0,200)
      draw_font(txt, off + 1, y+1 , bg[1], bg[2], bg[3], bg[4])
      draw_font(txt, off + 0, y, color[1], color[2], color[3], color[4])
      -- caPrint(txt, 0, y, 1)
    end
  end
  rl_pop_matrix();

  if self.customDraw then
     self:customDraw()
  end
end


function easyAddTest(desc)
  local ports = desc.ports
  local cases = desc.cases
  local Test = CombinatorialTest:extend()
  function Test:setup()
    self.cases = cases
    self.ports = ports
    self.customDraw = desc.customDraw
  end
  add_chip_instance(Test())
end

local function split_tokens(line)
  local tokens = {}
  for token in line:gmatch("%S+") do
    table.insert(tokens, token)
  end
  return tokens
end

function print_table(t, indent)
  indent = indent or 0
  local spacing = string.rep("  ", indent)

  for k, v in pairs(t) do
    if type(v) == "table" then
      print(spacing .. tostring(k) .. ":")
      print_table(v, indent + 1)
    else
      print(spacing .. tostring(k) .. ": " .. tostring(v))
    end
  end
end

function lazy_parse(txt)
  local ctx = {
    ports={},
    cases={},
  }
  for line in txt:gmatch("([^\n]*)") do
    if line ~= "" then  -- Skip empty matches
      local tokens = split_tokens(line)
      if tokens[1] == 'conn' then
        local port = {
          input=tokens[2] == 'in',
          width=math.tointeger(tokens[3]),
          name=tokens[4],
        }
        table.insert(ctx.ports, port)
        -- print_table(port)
      elseif tokens[1] == 'test' then
        local case = {}
        local np = #ctx.ports
        for i=1,np do
          local v = bits(tokens[1+i])
          local name = ctx.ports[i].name
          case[name] = v
        end
        local tname = tokens[np+2]
        for j=np+3, #tokens do
          tname = tname .. ' ' .. tokens[j]
        end
        case.name = tname
        table.insert(ctx.cases, case)
        -- print_table(case)
      end
    end
  end
  return ctx
end

function comb_level(txt, extra)
  local ctx = lazy_parse(txt)
  if extra ~= nil then
    ctx.customDraw = extra.customDraw
  end
  easyAddTest(ctx)
end
