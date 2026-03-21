
add_group({
  id="sevseg1",
  icon='sevenseg.png',
  name=T.sevseg_name,
  desc=T.sevseg_desc,
})

easy_add_level2({
  id="match7",
  group='sevseg1',
  deps={},
  extra_text= {
    {
      wiki='posint'
    }
  }
})

easy_add_level2({
  id="match7_or_23",
  group='sevseg1',
  deps={},
  extra_text= {}
})

easy_add_level2({
  id="decoder1",
  group='sevseg1',
  deps={},
  extra_text= {
    {
      wiki="decoder1"
    }
  }
})

easy_add_level2({
  id="decoder2",
  group='sevseg1',
  deps={'decoder1'},
  extra_text= {}
})

easy_add_level2({
  id="decoder_2bit",
  group='sevseg1',
  deps={},
  extra_text= {}
})

easy_add_level2({
  id="decoder3",
  group='sevseg1',
  deps={'decoder2'},
  extra_text= {}
})

easy_add_level2({
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


easy_add_level2({
  id="sevenseg",
  group='sevseg1',
  deps={'decoder2'},
  assets={
    'seven_seg_display.png',
  },
  extra_text= {
    {
      title=T.sevenseg_sample,
      img="levels/imgs/sevenseg1.png",
      scale=4,
    }
  }
})


