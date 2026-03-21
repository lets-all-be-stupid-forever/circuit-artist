
AddGroup({
  id='basics1',
  name=T.basics_name,
  icon='icons/campaign_basics1.png',
  desc=T.basics_desc
})

AddCampaignLevel({
  id="wires1",
  group='basics1',
  extra_text= { { wiki="wires1", } }
})

AddCampaignLevel({
  id="wires2",
  group='basics1',
  deps={'wires1'},
  extra_text= { { wiki="wires_crossing", } }
})

AddCampaignLevel({
  id="wires3",
  group='basics1',
  deps={'wires2'},
  extra_text= { { wiki="wires_io", } }
})

AddCampaignLevel({
  id="wires4",
  group='basics1',
  deps={'wires3'},
  extra_text= {
    {
      title=T.wires4_tip_title,
      text=T.wires4_tip_text,
    },
    {
      title=T.wires4_pin_title,
      text=T.wires4_pin_text,
    },
    {
      wiki="bit_order",
    }
  },
})

AddCampaignLevel({
  id="nand",
  group='basics1',
  deps={'wires1'},
  extra_text= {
    {
      title=T.nand_example_title,
      img="levels/imgs/nand_example.png",
      scale=8,
    },
    {
      title=T.nand_picture,
      img="levels/imgs/nand_schema.png",
      scale=4,
    },
    {
      wiki="nand",
    }
  }
})


AddCampaignLevel({
  id="not",
  group='basics1',
  deps={'nand'},
  extra_text= {
    {
      title=T.not_picture,
      img="levels/imgs/not_picture.png",
      scale=4,
    },
    {
      title=T.not_schema,
      img="levels/imgs/not_schema.png",
      scale=4,
    },
    { wiki="not", }
  }
})

AddCampaignLevel({
  id="and",
  group='basics1',
  deps={'not'},
  extra_text= {
    {
      title=T.and_picture,
      img="levels/imgs/and_picture.png",
      scale=4,
    },
    {
      title=T.and_schema,
      img="levels/imgs/and_schema.png",
      scale=4,
    },
    { wiki="and", }
  }
})


AddCampaignLevel({
  id="or",
  group='basics1',
  deps={'and'},
  extra_text= {
    {
      title=T.or_picture,
      img="levels/imgs/or_picture.png",
      scale=4,
    },
    {
      title=T.or_schema,
      img="levels/imgs/or_schema.png",
      scale=4,
    },
    { wiki="or", }
  }
})

AddCampaignLevel({
  id="xor",
  group='basics1',
  deps={'or'},
  extra_text= {
    {
      title=T.xor_picture,
      img="levels/imgs/xor_picture.png",
      scale=4,
    },
    {
      title=T.xor_schema,
      img="levels/imgs/xor_schema.png",
      scale=4,
    },
    { wiki="xor", }
  }
})


AddCampaignLevel({
  id="a_eq_b1",
  group='basics1',
  deps={'xor'},
  extra_text= {
    {
      title=T.a_eq_b1_tip,
      img="levels/imgs/a_eq_b1_tip.png",
      scale=4,
    }
  }
})


AddCampaignLevel({
  id="a_eq_b2",
  group='basics1',
  deps={'a_eq_b1', 'wires4'},
  extra_text= {
    {
      title=T.a_eq_b2_tip,
      text=T.a_eq_b2_tip_text,
    }
  }
})


