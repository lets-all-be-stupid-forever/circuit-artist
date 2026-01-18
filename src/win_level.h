#ifndef CA_WIN_LEVEL_H
#define CA_WIN_LEVEL_H
#include "game_registry.h"
#include "sim.h"
#include "status.h"

LevelAPI* getlevel();
void win_level_init(GameRegistry* r);
void win_level_open();
void win_level_set_campaign(int icampaign);
void win_level_update();
void win_level_draw();
Status level_load(LevelDef* ldef);
Status level_load_default();

#endif
