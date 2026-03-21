function AddCampaignLevel(ctx)
  AddLevel({
    id=ctx.id,
    group=ctx.group,
    name=T[ctx.id .. '_name'],
    deps=ctx.deps,
    icon="levels/icons/" .. ctx.id .. ".png",
    description=T[ctx.id .. '_desc'],
    kernel="levels/kernel/" .. ctx.id .. ".lua",
    extra_text=ctx.extra_text,
  })
end

----------------------------------------------------
Import 'locale/en.lua'
Import 'campaigns/custom.lua'
Import 'campaigns/basics1.lua'
Import 'campaigns/sevseg1.lua'
Import 'campaigns/routing1.lua'
Import 'campaigns/memory1.lua'
Import 'wikis.lua'
