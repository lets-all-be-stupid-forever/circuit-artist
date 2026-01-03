local function customDraw(self)

  -- TODO


end

local OP_NOP = 0
local OP_LOAD = 1
local OP_MOV = 2
local OP_NAND = 3

function initMachine()
  return {0, 0, 0, 0}
end

function runInstruction(m, I)
  if I.op == OP_NOP then
    return m
  elseif I.op == OP_LOAD then
    m[I.reg1 + 1] = I.data
    return m
  elseif I.op == OP_MOV then
    m[I.reg1 + 1] = m[I.reg2 + 1]
    return m
  elseif I.op == OP_NAND then
    m[1] = (~(m[1] & m[2])) & 1
  else
    error('Invalid OP')
  end
  return m
end

function dumpState(m)
  return '(y0=' .. m[1] .. ' y1=' .. m[2] .. ' y2=' .. m[3] .. ' y3=' .. m[4] .. ')'
end

function cleanCode(s)
  -- Returns code (before ;) and comment (after ;), both stripped
  local code, comment = s:match("^([^;]*);?(.*)")
  code = code and code:match("^%s*(.-)%s*$") or ""
  comment = comment and comment:match("^%s*(.-)%s*$") or ""
  return code, comment
end

local function parseRegister(s)
  -- Parse R0, R1, R2, R3 -> 0, 1, 2, 3
  -- Remove trailing comma if present
  s = s:gsub(",", "")
  local num = s:match("R(%d)")
  if not num then
    error("Invalid register: " .. s)
  end
  return tonumber(num)
end

local function parseImmediate(s)
  -- Remove trailing comma if present
  s = s:gsub(",", "")
  return tonumber(s)
end

function parseInstruction(s)
  --[[ Instruction syntax:
  LOAD Rx, imm   ; Load immediate (0 or 1) into Rx
  MOV Rx, Ry     ; Copy Ry into Rx
  NAND           ; R0 = ~(R0 & R1)

  Empty lines and comments (after ;) are treated as NOP.
  --]]

  -- Remove comments (everything after semicolon)
  local code = s:match("^([^;]*)") or ""

  -- Trim whitespace
  code = code:match("^%s*(.-)%s*$") or ""

  -- Empty line = NOP
  if code == "" then
    return {op = OP_NOP, reg1 = 0, reg2 = 0, data = 0}
  end

  -- Parse tokens
  local tokens = {}
  for token in code:gmatch("%S+") do
    table.insert(tokens, token:upper())
  end

  local opcode = tokens[1]

  if opcode == "NOP" then
    return {op = OP_NOP, reg1 = 0, reg2 = 0, data = 0}
  elseif opcode == "NAND" then
    return {op = OP_NAND, reg1 = 0, reg2 = 0, data = 0}
  elseif opcode == "LOAD" then
    local reg = parseRegister(tokens[2])
    local imm = parseImmediate(tokens[3])
    return {op = OP_LOAD, reg1 = reg, reg2 = 0, data = imm}
  elseif opcode == "MOV" then
    local dest = parseRegister(tokens[2])
    local src = parseRegister(tokens[3])
    return {op = OP_MOV, reg1 = dest, reg2 = src, data = 0}
  else
    error("Unknown instruction: " .. opcode)
  end
end

-- AND program: computes R3 = A AND B
-- Uses all 4 registers. AND = NOT(A NAND B) = (A NAND B) NAND (A NAND B)
local function andProgram(a, b)
  return {
    "LOAD R0, " .. a .. "  ; R0 = A",
    "LOAD R1, " .. b .. "  ; R1 = B",
    "MOV R2, R0            ; R2 = A (use R2)",
    "MOV R3, R1            ; R3 = B (use R3)",
    "NAND                  ; R0 = A NAND B",
    "MOV R1, R0            ; R1 = A NAND B",
    "NAND                  ; R0 = NOT(A NAND B) = A AND B",
    "MOV R3, R0            ; R3 = result",
  }
end

local function concatTables(...)
  local result = {}
  for _, t in ipairs({...}) do
    for _, v in ipairs(t) do
      table.insert(result, v)
    end
  end
  return result
end

-- Test all 4 input combinations: (0,0)->0, (0,1)->0, (1,0)->0, (1,1)->1
local program = concatTables(
  andProgram(0, 0),
  andProgram(0, 1),
  andProgram(1, 0),
  andProgram(1, 1)
)

local function makeCases()
  local cases = {
    {clk=0, rst=1, op=0, data=0, reg1=0, reg2=0, y=0, name='RESET', icode=0},
    {clk=1, rst=1, op=0, data=0, reg1=0, reg2=0, y=0, name='RESET', icode=0},
  }
  local m = initMachine()
  for i=1,#program do
    local code = program[i]
    local inst = parseInstruction(code)
    local s0 = dumpState(m)
    m.data = inst.data  -- Set data for LOAD instruction
    m = runInstruction(m, inst)
    local s1 = dumpState(m)
    local last = cases[#cases]
    local asm, cmt = cleanCode(code)
    local c0 = {
      rst=0,
      clk=0,
      data=inst.data,
      op=inst.op,
      reg1=inst.reg1,
      reg2=inst.reg2,
      y=last.y,
      name='I[' .. i .. ']: `' .. asm .. '`\n' .. s0,
      asm=asm,
      cmt=cmt,
      icode=i,
    }
    local c1 = {
      rst=0,
      clk=1,
      data=inst.data,
      op=inst.op,
      reg1=inst.reg1,
      reg2=inst.reg2,
      y=m[4],
      name='I[' .. i .. ']: `'.. asm .. '`\n' .. s0 .. ' -> ' .. s1,
      asm=asm,
      cmt=cmt,
      icode=i,
    }
    table.insert(cases, c0)
    table.insert(cases, c1)
  end
  return cases
end


easyAddTest({
  cases = makeCases(),
  customDraw=customDraw,
  ports = {
    {name="rst", width=1, input=false},
    {name="clk", width=1, input=false},
    {name="op", width=2, input=false},
    {name="data", width=1, input=false},
    {name="reg1", width=2, input=false},
    {name="reg2", width=2, input=false},
    {name="y", width=1, input=true},
  }
})

