
AddGroup({
  name=T.routing1_name,
  id='routing1',
  icon='icons/campaign_routing1.png',
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
      img="mux1.png",
      scale=4,
    },
    {
      title=T.mux_2_1_impl,
      img="mux2.png",
      scale=4,
    },
    {
      title=T.mux_2_1_analogy,
      img="mux3.png",
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
      img="mux4.png",
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
      img='bus_example.png',
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
      img='demux1.png',
      scale=4,
    },
    {
      title=T.demux_1_2_demux2,
      img='demux2.png',
      scale=4,
    },
    {
      title=T.demux_1_2_analogy3,
      img='demux3.png',
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
      img='demux4.png',
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
      img='router_example.png',
      scale=4,
    }
  }
})


