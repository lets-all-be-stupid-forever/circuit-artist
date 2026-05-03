#ifndef CA_WIN_CUSTOMLVL_H
#define CA_WIN_CUSTOMLVL_H
#include "game_registry.h"

#if defined(__cplusplus)
extern "C" {
#endif

void win_customlvl_init(GameRegistry* r);
void win_customlvl_open(CustomLevelDef* ldef);
void win_customlvl_update();
void win_customlvl_draw();

void notify_installed_steam_level(const char* folder, u64 item);

#if defined(__cplusplus)
}
#endif

#endif
