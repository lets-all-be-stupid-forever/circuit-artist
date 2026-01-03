#ifndef CA_STEAM_H
#define CA_STEAM_H
#include "common.h"
#include "game_registry.h"

#if defined(__cplusplus)
extern "C" {
#endif

bool SteamEnabled();

#ifdef WITH_STEAM
void SteamInit();
void SteamShutdown();
bool SteamGetAchievement(const char* ach_name);
void SteamSetAchievement(const char* ach_name);
void SteamClearAchievement(const char* ach_name);
void SteamRefreshAchievement();

int steam_get_stats(const char* name);
void steam_set_stats(const char* name, int v);
void steam_store_stats();

#endif

#if defined(__cplusplus)
}
#endif

#endif
