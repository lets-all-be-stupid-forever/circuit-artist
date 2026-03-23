function AddCampaignLevel(ctx)
  AddLevel({
    id=ctx.id,
    group=ctx.group,
    name=T[ctx.id .. '_name'],
    deps=ctx.deps,
    description=T[ctx.id .. '_desc'],
    kernel="levels/kernels/" .. ctx.id .. ".lua",
    extra_text=ctx.extra_text,
  })
end

Import 'locale/en.lua'
----------------------------------------------------
Import 'levels/custom.lua'
Import 'levels/basics1.lua'
Import 'levels/sevseg1.lua'
Import 'levels/routing1.lua'
Import 'levels/memory1.lua'
Import 'wiki/pages.lua'
