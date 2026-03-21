
add_group({
  id="custom",
  icon='sandbox.png',
  name=T.custom_name,
  desc=T.custom_desc,
})

easy_add_level2({
  id="sandbox",
  group='custom',
  extra_text={
    {
      title=T.sandbox_inputs_title,
      text=T.sandbox_inputs_text,
    },
  }
})

