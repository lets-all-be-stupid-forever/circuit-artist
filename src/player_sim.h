#ifndef CA_PLAYER_SIM_H
#define CA_PLAYER_SIM_H
#include "common.h"
#include "sim.h"

void player_sim_init();
void player_sim_start();
void player_sim_stop();
bool player_sim_did_compilation_fail();
bool player_sim_update();
bool player_sim_has_error();

#endif
