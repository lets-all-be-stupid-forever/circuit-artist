
function bits(s)
  return tonumber(s, 2)
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

function lazyParse(txt)
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
        print_table(port)
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
        print_table(case)
      end
    end
  end
end

t = [[
conn out 1 a
conn out 1 b
conn in 1 a_less_than_b
conn in 1 a_equal_b
conn in 1 a_greater_than_b
test 0 0 0 1 0
test 0 1 1 0 0
test 1 0 0 0 1
test 1 1 0 1 0
]]
lazyParse(t)
