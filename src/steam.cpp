#include "steam.h"

#include <stdio.h>

#ifdef WITH_STEAM
#include "steam/steam_api_flat.h"
#endif

bool SteamEnabled() {
#ifdef WITH_STEAM
  return true;
#else
  return false;
#endif
}

#ifdef WITH_STEAM

static struct {
  bool loaded;
  ISteamUserStats* user_stats;
  bool stats;
} C = {0};

void SteamInit() {
  C.loaded = SteamAPI_Init();
  if (C.loaded) {
    C.user_stats = SteamAPI_SteamUserStats();
    C.stats = SteamAPI_ISteamUserStats_RequestCurrentStats(C.user_stats);
    uint32 num_achievements =
        SteamAPI_ISteamUserStats_GetNumAchievements(C.user_stats);
    printf("STEAM loaded=%d stats=%d num_achievements=%d\n", C.loaded, C.stats,
           num_achievements);
  }
}

bool SteamGetAchievement(const char* ach_name) {
  bool ret = false;
  if (C.loaded) {
    bool s =
        SteamAPI_ISteamUserStats_GetAchievement(C.user_stats, ach_name, &ret);
    if (!s) {
      printf("Error in steam api!\n");
    }
  }
  return ret;
}

void SteamSetAchievement(const char* ach_name) {
  if (C.loaded) {
    bool s = SteamAPI_ISteamUserStats_SetAchievement(C.user_stats, ach_name);
    if (!s) {
      printf("Error in steam api!\n");
    }
  }
}

void SteamShutdown() {
  if (C.loaded) {
    SteamAPI_Shutdown();
  }
}

void SteamClearAchievement(const char* ach_name) {
  if (C.loaded) {
    bool s = SteamAPI_ISteamUserStats_ClearAchievement(C.user_stats, ach_name);
    if (!s) {
      printf("Error in steam api!\n");
    }
  }
}

void SteamRefreshAchievement() {
  if (C.loaded) {
    bool s = SteamAPI_ISteamUserStats_StoreStats(C.user_stats);
    if (!s) {
      printf("Error in steam api!\n");
    }
  }
}
#endif
