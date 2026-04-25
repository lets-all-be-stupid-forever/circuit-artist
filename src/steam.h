#ifndef CA_STEAM_H
#define CA_STEAM_H
#include "blueprint.h"
#include "common.h"
#include "game_registry.h"

#if defined(__cplusplus)
extern "C" {
#endif

bool SteamEnabled();
bool is_steam_on();

#ifdef WITH_STEAM
void SteamRunFrame();
void SteamInit();
void SteamShutdown();
bool SteamGetAchievement(const char* ach_name);
void SteamSetAchievement(const char* ach_name);
void SteamClearAchievement(const char* ach_name);
void SteamRefreshAchievement();

int steam_get_stats(const char* name);
void steam_set_stats(const char* name, int v);
void steam_store_stats();
bool steam_unsubscribe_item(u64 id);
void steam_open_overlay_item(u64 file_id);

#endif

void steam_open_overlay_blueprints();
void steam_load_blueprints(BlueprintStore* store);
void* steam_upload_item(const char* folder, const char* chg_notes,
                        const char* title, const char* desc,
                        const char* thumb_path, int ntags, const char** tags);

/* Returns true when it's over */
bool steam_upload_update(void* ctx);
void steam_upload_free(void* ctx);

#if defined(__cplusplus)
}
#endif

#endif
