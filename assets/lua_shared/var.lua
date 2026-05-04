local __ctx = {
   variables = {},
   changes = {},
}

function Var(v0)
  local idx = #__ctx.variables + 1
  __ctx.variables[idx] = v0
  return function(arg)
    if arg == nil then
      return __ctx.variables[idx]
    else
      local before = __ctx.variables[idx]
      local after = arg
      table.insert(__ctx.changes, {idx, before, after})
    end
  end
end

function ResetVariables()
  __ctx.variables = {}
end

function CommitVariables()
  local out = __ctx.changes
  __ctx.changes = {}
  if #out == 0 then
    return nil
  else
    return out
  end
end

function ForwardVariables(patch)
  if patch == nil then
    return
  end
  for i=1,#patch do
    local desc = patch[i]
    local idx = desc[1]
    local v1 = desc[3]
    __ctx.variables[idx] = v1
  end
end

function BackwardVariables(patch)
  if patch == nil then
    return
  end
  for i=1,#patch do
    local desc = patch[i]
    local idx = desc[1]
    local v0 = desc[2]
    __ctx.variables[idx] = v0
  end
end
