#ifndef CA_LEVELSIDEBAR_H
#define CA_LEVELSIDEBAR_H
#include "game_registry.h"

void level_sidebar_init();
void level_sidebar_set_lvl(LevelDef* ldef);
void level_sidebar_update(Rectangle region);
void level_sidebar_draw();
void level_sidebar_draw_legend();

#endif
