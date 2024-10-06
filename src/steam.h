#ifndef CA_STEAM_H
#define CA_STEAM_H
#include "defs.h"

#if defined(__cplusplus)
extern "C" {
#endif

CA_API bool SteamEnabled();

#ifdef WITH_STEAM
void SteamInit();
void SteamShutdown();
CA_API bool SteamGetAchievement(const char* ach_name);
CA_API void SteamSetAchievement(const char* ach_name);
CA_API void SteamClearAchievement(const char* ach_name);
CA_API void SteamRefreshAchievement();
#endif

#if defined(__cplusplus)
}
#endif

#endif

