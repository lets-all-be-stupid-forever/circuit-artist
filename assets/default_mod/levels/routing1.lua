
AddGroup({
  name=T.routing1_name,
  id='routing1',
  icon='levels/icons/campaign_routing1.png',
  desc=T.routing1_desc
})


AddCampaignLevel({
  id="mux_2_1",
  group='routing1',
  deps={},
  extra_text= {
    {
      wiki="mux",
    },
    {
      title=T.mux_2_1_schema,
      img="wiki/imgs/mux1.png",
      scale=4,
    },
    {
      title=T.mux_2_1_impl,
      img="wiki/imgs/mux2.png",
      scale=4,
    },
    {
      title=T.mux_2_1_analogy,
      img="wiki/imgs/mux3.png",
      scale=4,
    },
  }
})

AddCampaignLevel({
  id="mux_4_1",
  group='routing1',
  deps={},
  extra_text= {
    {
      title=T.mux_4_1_larger,
      text=T.mux_4_1_larger_text,
    },
    {
      title=T.mux_4_1_schema,
      img="levels/imgs/mux4.png",
      scale=4,
    },
  }
})

AddCampaignLevel({
  id="mux_4_2",
  group='routing1',
  deps={},
  extra_text={}
})

AddCampaignLevel({
  id="bus2",
  group='routing1',
  deps={},
  extra_text= {
    {
      title=T.bus2_example,
      text=T.bus2_example_text
    },
    {
      title=T.bus2_example2,
      img='levels/imgs/bus_example.png',
      scale=4,
    }
  }
})

AddCampaignLevel({
  id="demux_1_2",
  group='routing1',
  deps={},
  extra_text = {
    {
      wiki='demux',
    },
    {
      title=T.demux_1_2_analogy_title,
      text=T.demux_1_2_analogy_text,
    },
    {
      title=T.demux_1_2_analogy2_title,
      text=T.demux_1_2_analogy2_text,
    },
    {
      title=T.demux_1_2_demux,
      img='wiki/imgs/demux1.png',
      scale=4,
    },
    {
      title=T.demux_1_2_demux2,
      img='wiki/imgs/demux2.png',
      scale=4,
    },
    {
      title=T.demux_1_2_analogy3,
      img='wiki/imgs/demux3.png',
      scale=4,
    },
  }
})

AddCampaignLevel({
  id="demux_1_4",
  group='routing1',
  deps={},
  extra_text= {
    {
      title=T.demux_1_4_tip_title,
      img='levels/imgs/demux4.png',
      scale=4,
    },
  }
})

AddCampaignLevel({
  id="demux_2_4",
  group='routing1',
  deps={},
  extra_text= {}
})

AddCampaignLevel({
  id="router",
  group='routing1',
  deps={},
  extra_text= {
    {
      title=T.router_example_title,
      text=T.router_example_text,
    },
    {
      title=T.router_example2_title,
      img='levels/imgs/router_example.png',
      scale=4,
    }
  }
})


