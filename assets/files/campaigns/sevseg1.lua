
AddGroup({
  id="sevseg1",
  icon='icons/campaign_sevenseg1.png',
  name=T.sevseg_name,
  desc=T.sevseg_desc,
})

AddCampaignLevel({
  id="match7",
  group='sevseg1',
  deps={},
  extra_text= {
    {
      wiki='posint'
    }
  }
})

AddCampaignLevel({
  id="match7_or_23",
  group='sevseg1',
  deps={},
  extra_text= {}
})

AddCampaignLevel({
  id="decoder1",
  group='sevseg1',
  deps={},
  extra_text= {
    {
      wiki="decoder1"
    }
  }
})

AddCampaignLevel({
  id="decoder2",
  group='sevseg1',
  deps={'decoder1'},
  extra_text= {}
})

AddCampaignLevel({
  id="decoder_2bit",
  group='sevseg1',
  deps={},
  extra_text= {}
})

AddCampaignLevel({
  id="decoder3",
  group='sevseg1',
  deps={'decoder2'},
  extra_text= {}
})

AddCampaignLevel({
  id="match_many",
  group='sevseg1',
  deps={'decoder2'},
  extra_text= {
    {
      title=T.match_many_tip_title,
      text=T.match_many_tip_text,
    },
    {
      title=T.match_many_table_title,
      text=T.match_many_table_text
    },
  }
})


AddCampaignLevel({
  id="sevenseg",
  group='sevseg1',
  deps={'decoder2'},
  extra_text= {
    {
      title=T.sevenseg_sample,
      img="levels/imgs/sevenseg1.png",
      scale=4,
    }
  }
})


