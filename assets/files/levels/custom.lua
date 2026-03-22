
AddGroup({
  id="custom",
  icon='levels/icons/sandbox.png',
  name=T.custom_name,
  desc=T.custom_desc,
})

AddCampaignLevel({
  id="sandbox",
  group='custom',
  extra_text={
    {
      title=T.sandbox_inputs_title,
      text=T.sandbox_inputs_text,
    },
  }
})

