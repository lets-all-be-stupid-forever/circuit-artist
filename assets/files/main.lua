function easy_add_level2(ctx)
  if ctx.group == nil then
    ctx.group = 'custom'
  end
  if ctx.assets == nil then
    ctx.assets = {}
  end
  if ctx.content == nil then
    ctx.content = {
      "shared/classic.lua",
      "shared/preload_chip.lua",
      "shared/comb_level.lua",
      "levels/kernel/" .. ctx.id .. ".lua",
    }
  end

  add_level({
    id=ctx.id,
    group=ctx.group,
    name=T[ctx.id .. '_name'],
    assets=ctx.assets,
    deps=ctx.deps,
    icon="levels/icons/" .. ctx.id .. ".png",
    description=T[ctx.id .. '_desc'],
    content=ctx.content,
    extra_text=ctx.extra_text,
  })
end

----------------------------------------------------
Import 'en.lua'
Import 'custom_campaign.lua'
Import 'basics_campaign.lua'
Import 'sevseg_campaign.lua'
Import 'routing_campaign.lua'
Import 'memory_campaign.lua'
Import 'wikis.lua'
