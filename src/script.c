#include "script.h"

#include "common.h"
#include "game_registry.h"
#include "stb_ds.h"
#include "stdio.h"
#include "string.h"
#include "win_level.h"
#include "win_main.h"

static struct {
  bool running;
  int state;
  int cur_level;
  int wait;
} C = {0};

static struct {
  const char* id;
  const char* path;
} tests[] = {

    /* Campaign 1: Gates */
    {"wires1", "../campaign_solutions/01_basics1/wires1.png"},
    {"wires2", "../campaign_solutions/01_basics1/wires2.png"},
    {"wires3", "../campaign_solutions/01_basics1/wires3.png"},
    {"wires4", "../campaign_solutions/01_basics1/wires4.png"},
    {"nand", "../campaign_solutions/01_basics1/nand.png"},
    {"not", "../campaign_solutions/01_basics1/not.png"},
    {"and", "../campaign_solutions/01_basics1/and.png"},
    {"or", "../campaign_solutions/01_basics1/or.png"},
    {"xor", "../campaign_solutions/01_basics1/xor.png"},
    {"a_eq_b1", "../campaign_solutions/01_basics1/a_eq_b1.png"},
    {"a_eq_b2", "../campaign_solutions/01_basics1/a_eq_b2.png"},

    /* Campaign 2: Seven Seg */
    {"match7", "../campaign_solutions/02_sevseg1/match7.png"},
    {"match7_or_23", "../campaign_solutions/02_sevseg1/match7_or_23.png"},
    {"decoder1", "../campaign_solutions/02_sevseg1/decoder1.png"},
    {"decoder2", "../campaign_solutions/02_sevseg1/decoder2.png"},
    {"decoder_2bit", "../campaign_solutions/02_sevseg1/decoder_2bit.png"},
    {"decoder3", "../campaign_solutions/02_sevseg1/decoder3.png"},
    {"match_many", "../campaign_solutions/02_sevseg1/match_many.png"},
    {"sevenseg", "../campaign_solutions/02_sevseg1/sevenseg.png"},

    /* Campaign 3: Routing */
    {"mux_2_1", "../campaign_solutions/03_routing1/mux_2_1.png"},
    {"mux_4_1", "../campaign_solutions/03_routing1/mux_4_1.png"},
    {"mux_4_2", "../campaign_solutions/03_routing1/mux_4_2.png"},
    {"bus2", "../campaign_solutions/03_routing1/bus2.png"},
    {"demux_1_2", "../campaign_solutions/03_routing1/demux_1_2.png"},
    {"demux_1_4", "../campaign_solutions/03_routing1/demux_1_4.png"},
    {"demux_2_4", "../campaign_solutions/03_routing1/demux_2_4.png"},
    {"router", "../campaign_solutions/03_routing1/router.png"},

    /* Campaign 4: Memory */
    {"latch_door", "../campaign_solutions/04_memory1/latch_door.png"},
    {"dlatch", "../campaign_solutions/04_memory1/dlatch.png"},
    {"photo", "../campaign_solutions/04_memory1/photo.png"},
    {"combo_detector", "../campaign_solutions/04_memory1/combo_detector.png"},
    {"dflipflop_with_enable",
     "../campaign_solutions/04_memory1/dflipflop_with_enable.png"},
    {"dff_w_r", "../campaign_solutions/04_memory1/dff_w_r.png"},
    {"register4", "../campaign_solutions/04_memory1/register4.png"},
    {"registerfile", "../campaign_solutions/04_memory1/registerfile.png"},
    {"npu1", "../campaign_solutions/04_memory1/npu1.png"},

    /* Campaign 5 : Basic Math  */
    {"halfadder", "../campaign_solutions/05_aplusb/halfadder.png"},
    {"fulladder", "../campaign_solutions/05_aplusb/fulladder.png"},
    {"adder4bit", "../campaign_solutions/05_aplusb/adder4bit.png"},
    {"nega", "../campaign_solutions/05_aplusb/nega.png"},
    {"subtractor", "../campaign_solutions/05_aplusb/subtractor.png"},
    {"comparator1", "../campaign_solutions/05_aplusb/comparator1.png"},
    {"comparator2", "../campaign_solutions/05_aplusb/comparator2.png"},
    {"comparator3", "../campaign_solutions/05_aplusb/comparator3.png"},
    {"amul2", "../campaign_solutions/05_aplusb/amul2.png"},
    {"amul3", "../campaign_solutions/05_aplusb/amul3.png"},
    {"amulb", "../campaign_solutions/05_aplusb/amulb.png"},

    {NULL, NULL}};

enum {
  STATE_OPEN_LEVEL_WIN = 0,
  STATE_SELECT_LEVEL,
  STATE_CLOSE_LEVEL_WIN,
  STATE_LOAD_IMAGE,
  STATE_START_SIMU,
  STATE_MONITOR_PROGRESS,
  STATE_STOP_SIMU,
  STATE_NEXT_LEVEL,
  STATE_DONE,
  STATE_FAILURE,
};

void script_update() {
  if (!C.running) {
    if (IsKeyPressed(KEY_F8)) {
      C.running = true;
    }
    C.state = 0;
    C.cur_level = 0;
    return;
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    C.running = false;
    C.state = STATE_FAILURE;
    return;
  }
  GameRegistry* r = getreg();

  if (C.wait > 0) {
    C.wait--;
    return;
  }
  switch (C.state) {
    case STATE_DONE: {
      C.running = false;
      return;
    };
    case STATE_OPEN_LEVEL_WIN: {
      win_main_open_level();
      C.wait = 5;
      C.state++;
      break;
    }

    case STATE_SELECT_LEVEL: {
      if (tests[C.cur_level].id == NULL) {
        C.state = STATE_DONE;
        break;
      }
      char txt[200];
      snprintf(txt, sizeof(txt), "campaign:%s", tests[C.cur_level].id);
      LevelDef* ldef = get_level_by_id(r, txt);
      win_level_set_sel(ldef);
      C.wait = 5;
      C.state++;
      break;
    }
    case STATE_CLOSE_LEVEL_WIN: {
      win_level_accept();
      C.wait = 5;
      C.state++;
      break;
    }
    case STATE_LOAD_IMAGE: {
      win_main_load_image_from_path(tests[C.cur_level].path);
      C.wait = 5;
      C.state++;
      break;
    }
    case STATE_START_SIMU: {
      win_main_start_simu();
      C.wait = 5;
      C.state++;
      break;
    }
    case STATE_MONITOR_PROGRESS: {
      bool error = win_main_is_simu_error();
      bool done = win_main_is_simu_done();
      if (error) {
        C.state = STATE_FAILURE;
        printf("ERROR: Error in level %s", tests[C.cur_level].id);
      } else if (done) {
        C.state++;
      }
      C.wait = 5;
      break;
    }
    case STATE_STOP_SIMU: {
      win_main_stop_simu();
      C.wait = 5;
      C.state++;
      break;
    }
    case STATE_NEXT_LEVEL: {
      C.state = 0;
      C.cur_level++;
      C.wait = 5;
      break;
    }
    case STATE_FAILURE: {
      C.wait = 50;
      break;
    }
  }
}
