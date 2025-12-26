-- Module to parse text content.
-- Basically will load external assets referenced in text and format it
-- before passing to C.
local R = require 'raylib_api'
local ffi = require 'ffi'
local utils = require 'utils'

-- module
local tb = {}

local function toSprite(img)
  if type(img)== 'string' then
    local tex = R.LoadTexture(img)
    img = { tex, 0, 0, tex.width, tex.height, }
  end

  local s = ffi.new('Sprite')
  s.tex = img[1]
  local x = img[2]
  local y = img[3]
  local w = img[4]
  local h = img[5]
  s.region = ffi.new('Rectangle', {x,y,w,h})
  return s
end

-- Transforms a raw text into {text, imgs} pair to be used in
-- draw_textbox
function tb.prepareText(text)
  local array = {}
  local root = utils.scriptPath()
  i = 0
  for capture in string.gmatch(text, "!img:([^%s]*)%s") do
    i= i+1
    table.insert(array, capture)
  end
  local sprites = {}
  local out = text
  for i=1,#array do
    local path = array[i]
    local t = R.LoadTexture(root .. path)
    if not (t.width > 0) then
      print('bad image:', root .. path)
    end
    assert (t.width > 0)
    table.insert(sprites, toSprite({t, 0, 0, t.width, t.height}))
    out = out:gsub("!img:" .. path, '!img:'.. i-1)
  end
  return {
    text=out,
    sprites=sprites,
  }
end

return tb
