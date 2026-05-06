#ifndef CA_STEAM_H
#define CA_STEAM_H
#include "blueprint.h"
#include "common.h"
#include "game_registry.h"
#include "win_pubform.h"

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

#endif

enum {
  ITEM_STATE_NONE = 0,
  ITEM_STATE_SUBSCRIBED = 1,
  ITEM_STATE_INSTALLED = 2,
  ITEM_STATE_DOWNLOADING = 4,
  ITEM_STATE_DOWNLOADPENDING = 8,
  ITEM_STATE_NEEDSUPDATE = 16,
};

u32 steam_item_state(u64 id);

void steam_open_overlay_item(u64 file_id);
bool steam_unsubscribe_item(u64 id);
void steam_subscribe_item(u64 id);
void steam_open_overlay_solutions(const char* level_id);
void steam_open_overlay_blueprints();
void steam_load_blueprints_and_levels();
void* steam_upload_item(const char* folder, const char* chg_notes,
                        const char* title, const char* desc,
                        const char* thumb_path, int ntags, const char** tags,
                        int nkvtags, const char** kvtags);

/* Returns true when it's over */
bool steam_upload_update(void* ctx);
void steam_upload_free(void* ctx);
void steam_browse_workshop_levels();

// void* steam_http_call(const char* url);
// bool steam_http_update(void* ctx);
// void steam_http_get_data_as_image(void* ctx, Image* img);
// void steam_http_destroy(void* ctx);
// void* steam_query_call_all(const char* txt);

typedef struct {
  char* url;
  int votes_up;
  int votes_down;
  u64 owner_id;
  u64 file_id;
  char* title;
  char* desc;
  char* author_name;
} QueryResultItem;

typedef struct {
  bool ok;
  int page;
  int total;
  int num_items;
  int page_size;
  QueryResultItem* items;
} QueryResult;

typedef enum {
  WORKSHOP_SORT_TRENDING = 0,
  WORKSHOP_SORT_RECENT,
  WORKSHOP_SORT_VOTES,
  WORKSHOP_SORT_TEXT,
} WorkshopSortMode;

void* steam_query_call_all(const char* search_text, int page, WorkshopSortMode sort);
void* steam_query_solution(const char* level_id, int page, WorkshopSortMode sort);
void* steam_query_subscribed(int page);
void* steam_query_my_uploads(int page);
bool steam_query_update(void* c, QueryResult* r);
void steam_query_destroy(void* c);
void steam_query_call_cancel(void* ctx);
void unload_query_results(QueryResult* r);
const char* steam_get_author_name(u64 steam_id);

void* steam_http_call(const char* url);
bool steam_http_update(void* c, Image* img);
void steam_http_cancel(void* c);
void steam_http_destroy(void* c);

#if defined(__cplusplus)
}
#endif

#endif
