#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4267)  // size_t to int
#endif

#include "steam.h"

#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#include "fs.h"
#include "stb_ds.h"
#include "utils.h"
#include "win_blueprint.h"
#include "win_customlvl.h"

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
  ISteamUGC* ugc;
  AppId_t app_id;
  AccountID_t my_id;
  std::string my_name;
  bool stats;
} C = {0};

class SteamSubscriptionListener {
 public:
  STEAM_CALLBACK(SteamSubscriptionListener, OnSubscribed,
                 RemoteStoragePublishedFileSubscribed_t);
  STEAM_CALLBACK(SteamSubscriptionListener, OnUnsubscribed,
                 RemoteStoragePublishedFileUnsubscribed_t);
  STEAM_CALLBACK(SteamSubscriptionListener, OnDeleted,
                 RemoteStoragePublishedFileDeleted_t);
  STEAM_CALLBACK(SteamSubscriptionListener, OnDownloaded, DownloadItemResult_t);
  STEAM_CALLBACK(SteamSubscriptionListener, OnInstalled, ItemInstalled_t);
};

void SteamRunFrame() {
  if (C.loaded) {
    SteamAPI_RunCallbacks();
  }
}

static bool meta_has_tag(SteamMeta* m, const char* tag) {
  for (int i = 0; i < m->ntags; i++) {
    if (strcmp(m->tags[i], tag) == 0) {
      return true;
    }
  }
  return false;
}

static bool folder_is_blueprint(const char* folder) {
  bool ok = true;
  ok = ok && os_path_exists(TextFormat("%s/full.png", folder));
  ok = ok && os_path_exists(TextFormat("%s/meta.json", folder));
  ok = ok && os_path_exists(TextFormat("%s/thumb.png", folder));
  return ok;
}

void process_item_ingress(u64 id) {
  uint32 state = C.ugc->GetItemState(id);
  bool subscribed = (state & k_EItemStateSubscribed);
  bool installed = (state & k_EItemStateInstalled);
  if (installed && subscribed) {
    uint64 sizeOnDisk;
    char folder[512];
    uint32 timestamp;
    bool ok = C.ugc->GetItemInstallInfo(id, &sizeOnDisk, folder, sizeof(folder),
                                        &timestamp);
    if (!ok) return;
    SteamMeta meta = {0};
    ok = load_steam_metadata(folder, &meta);
    if (ok) {
      if (folder_is_blueprint(folder)) {
        add_steam_blueprint(folder, id);
      } else if (folder_is_level(folder)) {
        notify_installed_steam_level(folder, id);
      }
      unload_steam_meta(&meta);
    }
  }
}

void SteamSubscriptionListener::OnSubscribed(
    RemoteStoragePublishedFileSubscribed_t* cb) {
  if (cb->m_nAppID != C.app_id) return;
  printf("[steam] subscribed: %" PRIu64 "\n",
         (unsigned long long)cb->m_nPublishedFileId);
  uint32 s = SteamAPI_ISteamUGC_GetItemState(C.ugc, cb->m_nPublishedFileId);
  bool installed = (s & k_EItemStateInstalled);
  if (!installed) {
    SteamAPI_ISteamUGC_DownloadItem(C.ugc, cb->m_nPublishedFileId, true);
    return;
  }
  process_item_ingress(cb->m_nPublishedFileId);
}

void SteamSubscriptionListener::OnUnsubscribed(
    RemoteStoragePublishedFileUnsubscribed_t* cb) {
#if 0
  if (cb->m_nAppID != C.app_id) return;
  printf("[steam] unsubscribed: %llu\n",
         (unsigned long long)cb->m_nPublishedFileId);
  u64 id = cb->m_nPublishedFileId;
  notify_item_uninstalled(id);
#endif
}

void SteamSubscriptionListener::OnDeleted(
    RemoteStoragePublishedFileDeleted_t* cb) {
#if 0
  if (cb->m_nAppID != C.app_id) return;
  printf("[steam] deleted: %llu\n", (unsigned long long)cb->m_nPublishedFileId);
  u64 id = cb->m_nPublishedFileId;
  notify_item_uninstalled(id);
#endif
}

void SteamSubscriptionListener::OnDownloaded(DownloadItemResult_t* cb) {
#if 0
  if (cb->m_unAppID != C.app_id) return;
  printf("[steam] downloaded: %llu (result=%d)\n",
         (unsigned long long)cb->m_nPublishedFileId, (int)cb->m_eResult);
  process_item_ingress(cb->m_nPublishedFileId);
#endif
}

void SteamSubscriptionListener::OnInstalled(ItemInstalled_t* cb) {
  if (cb->m_unAppID != C.app_id) return;
  printf("[steam] installed: %" PRIu64 "\n",
         (unsigned long long)cb->m_nPublishedFileId);
  process_item_ingress(cb->m_nPublishedFileId);
}

static SteamSubscriptionListener* g_sub_listener = nullptr;

void SteamInit() {
  C.loaded = SteamAPI_Init();
  if (C.loaded) {
    C.user_stats = SteamUserStats();
    C.ugc = SteamUGC();
    C.app_id = SteamAPI_ISteamUtils_GetAppID(SteamAPI_SteamUtils());
    C.my_id = SteamUser()->GetSteamID().ConvertToUint64() & 0xFFFFFFFF;
    C.my_name = SteamFriends()->GetPersonaName();
    C.stats = C.loaded;
    g_sub_listener = new SteamSubscriptionListener();
    uint32 num_achievements = C.user_stats->GetNumAchievements();
    printf("----------------------\n");
    printf("STEAM loaded=%d stats=%d num_achievements=%d\n", C.loaded, C.stats,
           num_achievements);
    printf("----------------------\n");
  } else {
    printf("----------------------\n");
    printf("COULDN'T LOAD STEAM :(\n");
    printf("----------------------\n");
  }
}

bool SteamGetAchievement(const char* ach_name) {
  bool ret = false;
  if (C.loaded) {
    bool s = C.user_stats->GetAchievement(ach_name, &ret);
    if (!s) {
      printf("Error in steam api!\n");
    }
  }
  return ret;
}

void SteamSetAchievement(const char* ach_name) {
  if (C.loaded) {
    bool s = C.user_stats->SetAchievement(ach_name);
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
    bool s = C.user_stats->ClearAchievement(ach_name);
    if (!s) {
      printf("Error in steam api!\n");
    }
  }
}

void SteamRefreshAchievement() {
  if (C.loaded) {
    bool s = C.user_stats->StoreStats();
    if (!s) {
      printf("Error in steam api!\n");
    }
  }
}

int steam_get_stats(const char* name) {
  int r = 0;
  bool ok = C.user_stats->GetStat(name, &r);
  if (!ok) printf("Error getting stat for %s\n", name);
  return r;
}

void steam_set_stats(const char* name, int v) {
  bool ok = C.user_stats->SetStat(name, v);
  if (!ok) printf("Error getting setting stat for %s\n", name);
}

void steam_store_stats() {
  bool s = C.user_stats->StoreStats();
  if (!s) {
    printf("Couldn't store stats!\n");
  }
}

#endif

bool is_steam_on() {
#ifdef WITH_STEAM
  return C.loaded;
#else
  return false;
#endif
}

#ifdef WITH_STEAM
const char* steam_err_to_str(EResult s) {
  switch (s) {
    case k_EResultOK:
      return "Success";
    case k_EResultFail:
      return "Generic failure";
    case k_EResultInvalidParam:
      return "Invalid parameter";
    case k_EResultAccessDenied:
      return "Access denied";
    case k_EResultNotLoggedOn:
      return "Not logged on";
    case k_EResultBanned:
      return "Banned";
    case k_EResultTimeout:
      return "Timeout";
    case k_EResultInsufficientPrivilege:
      return "Insufficient privilege (banned from Workshop)";
    case k_EResultLimitExceeded:
      return "Limit exceeded";
    case k_EResultFileNotFound:
      return "File not found";
    case k_EResultDuplicateRequest:
      return "Duplicate request";
    case k_EResultServiceUnavailable:
      return "Service unavailable";
    default:
      return "Unknown error";
  }
}
#endif

void steam_open_overlay_item(u64 file_id) {
#ifdef WITH_STEAM
  char url[512];
  snprintf(url, sizeof(url), "steam://url/CommunityFilePage/%" PRIu64, file_id);
  SteamAPI_ISteamFriends_ActivateGameOverlayToWebPage(
      SteamAPI_SteamFriends(), url,
      k_EActivateGameOverlayToWebPageMode_Default);
#endif
}

#ifdef WITH_STEAM
/* Calls api twice: once for create item, other for the actual upload.
 * */
struct SteamUploadContext {
  std::string title;
  std::string desc;
  std::string thumbnail_path;
  std::string folder;
  std::string chg_notes;
  std::vector<std::string> tags;
  std::vector<std::string> kv_keys;
  std::vector<std::string> kv_vals;
  SteamAPICall_t api_call;
  UGCUpdateHandle_t handle;
  bool create_pending = false;
  bool update_pending = false;
  u64 file_id;
  EResult error;

  void LaunchCreateItem() {
    api_call = SteamAPI_ISteamUGC_CreateItem(C.ugc, C.app_id,
                                             k_EWorkshopFileTypeCommunity);
    create_pending = true;
  }

  void UpdateCreateItem() {
    CreateItemResult_t result;
    bool failed;
    bool done = SteamAPI_ISteamUtils_GetAPICallResult(
        SteamAPI_SteamUtils(), api_call, &result, sizeof(result),
        3403,  // CreateItemResult_t
        &failed);
    if (!done) {
      create_pending = true;
      return;
    }
    if (!failed && result.m_eResult == k_EResultOK) {
      file_id = result.m_nPublishedFileId;
      create_pending = false;
      LaunchUpdateItem();
      return;
    } else {
      create_pending = false;
      error = result.m_eResult;
      printf("Create Item failure!!\n");
    }
  }

  void LaunchUpdateItem() {
    handle = SteamAPI_ISteamUGC_StartItemUpdate(C.ugc, C.app_id, file_id);
    if (!title.empty()) {
      SteamAPI_ISteamUGC_SetItemTitle(C.ugc, handle, title.c_str());
    }
    if (!desc.empty()) {
      SteamAPI_ISteamUGC_SetItemDescription(C.ugc, handle, desc.c_str());
    }
    SteamAPI_ISteamUGC_SetItemContent(C.ugc, handle, folder.c_str());
    if (!thumbnail_path.empty()) {
      SteamAPI_ISteamUGC_SetItemPreview(C.ugc, handle, thumbnail_path.c_str());
    }

    std::vector<const char*> s_tags;
    for (size_t i = 0; i < tags.size(); i++) {
      s_tags.push_back(tags[i].c_str());
    }

    SteamParamStringArray_t p_tags = {0};
    p_tags.m_ppStrings = s_tags.data();
    p_tags.m_nNumStrings = s_tags.size();
    bool allow_admin_tags = false;
    SteamAPI_ISteamUGC_SetItemTags(C.ugc, handle, &p_tags, allow_admin_tags);

    std::vector<const char*> s_kv_keys, s_kv_vals;
    for (size_t i = 0; i < kv_keys.size(); i++) {
      s_kv_keys.push_back(kv_keys[i].c_str());
      s_kv_vals.push_back(kv_vals[i].c_str());
    }
    SteamAPI_ISteamUGC_RemoveAllItemKeyValueTags(C.ugc, handle);
    for (size_t i = 0; i < s_kv_keys.size(); i++) {
      SteamAPI_ISteamUGC_AddItemKeyValueTag(C.ugc, handle, s_kv_keys[i],
                                            s_kv_vals[i]);
    }

    SteamAPI_ISteamUGC_SetItemVisibility(
        C.ugc, handle, k_ERemoteStoragePublishedFileVisibilityPublic);

    save_steam_metadata(folder.c_str(), C.my_id, C.my_name.c_str(),
                        s_tags.size(), s_tags.data(), s_kv_keys.size(),
                        s_kv_keys.data(), s_kv_vals.data());

    api_call =
        SteamAPI_ISteamUGC_SubmitItemUpdate(C.ugc, handle, chg_notes.c_str());
    update_pending = true;
  }

  void UpdateUpdateItem() {
    SubmitItemUpdateResult_t result;
    bool failed = false;
    bool done = SteamAPI_ISteamUtils_GetAPICallResult(
        SteamAPI_SteamUtils(), api_call, &result, sizeof(result),
        3404,  // SubmitItemUpdateResult_t_k_iCallback
        &failed);
    if (!done) {
      // c->update_status = SteamAPI_ISteamUGC_GetItemUpdateProgress(
      //     C.ugc, c->handle, &c->bytes_done, &c->bytes_total);
      update_pending = true;
      return;
    }
    remove_steam_metadata(folder.c_str());
    if (!failed && result.m_eResult == k_EResultOK) {
      printf("Success uploading!\n");
      update_pending = false;
      steam_open_overlay_item(file_id);
      return;
    }
    printf("ItemUPdate failure!! err: %s\n",
           steam_err_to_str(result.m_eResult));
    // printf(
    //     "SubmitItemUpdate failed: eResult=%d failed=%d
    //     needsLegalAgreement=%d\n", (int)c->result.m_eResult, (int)c->failed,
    //     (int)c->result.m_bUserNeedsToAcceptWorkshopLegalAgreement);
    update_pending = false;
    error = result.m_eResult;
  }

  bool Update() {
    if (create_pending) {
      UpdateCreateItem();
    } else if (update_pending) {
      UpdateUpdateItem();
    } else {
      return true;
    }
    return false;
  }
};
#endif

void* steam_upload_item(const char* folder, const char* chg_notes,
                        const char* title, const char* desc,
                        const char* thumb_path, int ntags, const char** tags,
                        int nkvtags, const char** kvtags) {
#ifdef WITH_STEAM
  SteamUploadContext* ctx = new SteamUploadContext();
  if (title) ctx->title = title;
  if (desc) ctx->desc = desc;
  if (chg_notes) ctx->chg_notes = chg_notes;
  if (thumb_path) ctx->thumbnail_path = thumb_path;
  for (int i = 0; i < ntags; i++) {
    ctx->tags.push_back(tags[i]);
  }
  for (int i = 0; i < nkvtags; i++) {
    ctx->kv_keys.push_back(kvtags[2 * i + 0]);
    ctx->kv_vals.push_back(kvtags[2 * i + 1]);
  }
  ctx->folder = folder;
  ctx->LaunchCreateItem();
  return ctx;
#else
  return NULL;
#endif
}

/* Returns true when it's over */
bool steam_upload_update(void* ctx) {
#ifdef WITH_STEAM
  return ((SteamUploadContext*)ctx)->Update();
#endif
  return false;
}

void steam_upload_free(void* ctx) {
#ifdef WITH_STEAM
  SteamUploadContext* u = (SteamUploadContext*)ctx;
  delete u;
#endif
}

void steam_load_blueprints_and_levels() {
#ifdef WITH_STEAM
  bool include_locally_disabled = false;
  uint32 count =
      SteamAPI_ISteamUGC_GetNumSubscribedItems(C.ugc, include_locally_disabled);
  PublishedFileId_t* items =
      (PublishedFileId_t*)calloc(count, sizeof(PublishedFileId_t));
  uint32 returned = SteamAPI_ISteamUGC_GetSubscribedItems(
      C.ugc, items, count, include_locally_disabled);
  for (uint32 i = 0; i < returned; i++) {
    process_item_ingress(items[i]);
  }
  free(items);
#endif
}

bool steam_unsubscribe_item(u64 id) {
#ifdef WITH_STEAM
  return C.ugc->UnsubscribeItem(id);
#endif
}

void steam_open_overlay_blueprints() {
#ifdef WITH_STEAM
  char url[256];
  snprintf(url, sizeof(url),
           "https://steamcommunity.com/workshop/browse/"
           "?appid=%u&requiredtags%%5B%%5D=blueprint",
           C.app_id);
  SteamFriends()->ActivateGameOverlayToWebPage(
      url, k_EActivateGameOverlayToWebPageMode_Default);
#endif
}

void steam_browse_workshop_levels() {
#ifdef WITH_STEAM
  char url[256];
  snprintf(url, sizeof(url),
           "https://steamcommunity.com/workshop/browse/"
           "?appid=%u&requiredtags%%5B%%5D=level",
           C.app_id);
  SteamFriends()->ActivateGameOverlayToWebPage(
      url, k_EActivateGameOverlayToWebPageMode_Default);
#endif
}

#if 1
void steam_open_overlay_solutions(const char* level_id) {
#ifdef WITH_STEAM
  char url[512];
  snprintf(url, sizeof(url),
           "https://steamcommunity.com/workshop/browse/"
           "?appid=%u&requiredtags%%5B%%5D=blueprint&searchtext=%s",
           C.app_id, level_id ? level_id : "");
  SteamFriends()->ActivateGameOverlayToWebPage(
      url, k_EActivateGameOverlayToWebPageMode_Default);
#endif
}
#endif

#if 0
void steam_open_overlay_browse_solutions(const char* level_id) {
#ifdef WITH_STEAM
  if (!level_id) return;
  /* Build a per-level tag matching the one the publisher stamps on the
   * blueprint: "sol_<level_id>" with ':' → '_'. The overlay browse URL only
   * filters on plain tags, not KV tags, so we rely on this dedicated tag to
   * narrow results to the current level. */
  char sol_tag[256];
  int n = snprintf(sol_tag, sizeof(sol_tag), "sol_%s", level_id);
  if (n < 0) return;
  for (int i = 0; sol_tag[i]; i++) {
    if (sol_tag[i] == ':') sol_tag[i] = '_';
  }

  char url[512];
  snprintf(url, sizeof(url),
           "https://steamcommunity.com/workshop/browse/"
           "?appid=%u&requiredtags%%5B%%5D=%s",
           C.app_id, sol_tag);
  SteamAPI_ISteamFriends_ActivateGameOverlayToWebPage(
      SteamAPI_SteamFriends(), url,
      k_EActivateGameOverlayToWebPageMode_Default);
#endif
}
#endif

#ifdef WITH_STEAM
typedef struct {
  bool done;
  bool failed;
  bool canceled;
  UGCQueryHandle_t handle;
  SteamAPICall_t api_call;
  SteamUGCQueryCompleted_t result;
  int page;
} SteamQueryCall;

static SteamQueryCall* begin_query(int page, WorkshopSortMode sort) {
  static const EUGCQuery sort_map[] = {
      k_EUGCQuery_RankedByTrend,            // WORKSHOP_SORT_TRENDING
      k_EUGCQuery_RankedByPublicationDate,  // WORKSHOP_SORT_RECENT
      k_EUGCQuery_RankedByVote,             // WORKSHOP_SORT_VOTES
      k_EUGCQuery_RankedByTextSearch,       // WORKSHOP_SORT_TEXT
  };
  SteamQueryCall* c = (SteamQueryCall*)calloc(1, sizeof(SteamQueryCall));
  c->handle = SteamAPI_ISteamUGC_CreateQueryAllUGCRequestPage(
      C.ugc, sort_map[sort], k_EUGCMatchingUGCType_Items_ReadyToUse, 0,
      C.app_id, page);
  SteamAPI_ISteamUGC_SetAllowCachedResponse(C.ugc, c->handle, 0);
  SteamAPI_ISteamUGC_SetReturnMetadata(C.ugc, c->handle, true);
  SteamAPI_ISteamUGC_AddRequiredTag(C.ugc, c->handle, "blueprint");
  c->page = page;
  return c;
}
#endif

void* steam_query_call_all(const char* search_text, int page,
                           WorkshopSortMode sort) {
#ifdef WITH_STEAM
  SteamQueryCall* c = begin_query(page, sort);
  if (search_text && search_text[0]) {
    SteamAPI_ISteamUGC_SetSearchText(C.ugc, c->handle, search_text);
  }
  c->api_call = SteamAPI_ISteamUGC_SendQueryUGCRequest(C.ugc, c->handle);
  return c;
#else
  return NULL;
#endif
}

void* steam_query_solution(const char* level_id, int page,
                           WorkshopSortMode sort) {
#ifdef WITH_STEAM
  SteamQueryCall* c = begin_query(page, sort);
  SteamAPI_ISteamUGC_AddRequiredTag(C.ugc, c->handle, "solution");
  SteamAPI_ISteamUGC_AddRequiredKeyValueTag(C.ugc, c->handle, "solution_to",
                                            level_id);
  c->api_call = SteamAPI_ISteamUGC_SendQueryUGCRequest(C.ugc, c->handle);
  return c;
#else
  return NULL;
#endif
}

void* steam_query_subscribed(int page) {
#ifdef WITH_STEAM
  SteamQueryCall* c = (SteamQueryCall*)calloc(1, sizeof(SteamQueryCall));
  c->handle = SteamAPI_ISteamUGC_CreateQueryUserUGCRequest(
      C.ugc, C.my_id, k_EUserUGCList_Subscribed,
      k_EUGCMatchingUGCType_Items_ReadyToUse,
      k_EUserUGCListSortOrder_SubscriptionDateDesc, C.app_id, C.app_id, page);
  SteamAPI_ISteamUGC_SetReturnMetadata(C.ugc, c->handle, true);
  SteamAPI_ISteamUGC_AddRequiredTag(C.ugc, c->handle, "blueprint");
  c->api_call = SteamAPI_ISteamUGC_SendQueryUGCRequest(C.ugc, c->handle);
  c->page = page;
  return c;
#else
  return NULL;
#endif
}

void* steam_query_my_uploads(int page) {
#ifdef WITH_STEAM
  SteamQueryCall* c = (SteamQueryCall*)calloc(1, sizeof(SteamQueryCall));
  c->handle = SteamAPI_ISteamUGC_CreateQueryUserUGCRequest(
      C.ugc, C.my_id, k_EUserUGCList_Published,
      k_EUGCMatchingUGCType_Items_ReadyToUse,
      k_EUserUGCListSortOrder_CreationOrderDesc, C.app_id, C.app_id, page);
  SteamAPI_ISteamUGC_SetReturnMetadata(C.ugc, c->handle, true);
  SteamAPI_ISteamUGC_AddRequiredTag(C.ugc, c->handle, "blueprint");
  c->api_call = SteamAPI_ISteamUGC_SendQueryUGCRequest(C.ugc, c->handle);
  c->page = page;
  return c;
#else
  return NULL;
#endif
}

bool steam_query_update(void* ctx, QueryResult* r) {
#ifdef WITH_STEAM
  SteamQueryCall* c = (SteamQueryCall*)ctx;

  if (c->canceled) {
    r->ok = false;
    return true;
  }

  // Already cancelled or completed
  if (c->done || c->handle == k_UGCQueryHandleInvalid) {
    r->ok = false;
    return true;
  }

  c->done = SteamAPI_ISteamUtils_GetAPICallResult(
      SteamAPI_SteamUtils(), c->api_call, &c->result, sizeof(c->result),
      3401,  // SteamUGCQueryCompleted_t_k_iCallback
      &c->failed);

  if (!c->done) {
    return false;
  }

  if (c->failed || c->result.m_eResult != k_EResultOK) {
    printf("API failure!! err: %s\n", steam_err_to_str(c->result.m_eResult));
    SteamAPI_ISteamUGC_ReleaseQueryUGCRequest(C.ugc, c->handle);
    c->handle = k_UGCQueryHandleInvalid;
    r->ok = false;
    return true;
  }

  uint32 count = c->result.m_unNumResultsReturned;
  uint32 total = c->result.m_unTotalMatchingResults;  // for pagination
  *r = QueryResult{};
  r->ok = true;
  r->page_size = 50;
  r->total = total;
  r->page = c->page;
  r->num_items = count;
  for (uint32 i = 0; i < count; i++) {
    QueryResultItem q = {0};
    // Get basic details
    SteamUGCDetails_t details;
    SteamAPI_ISteamUGC_GetQueryUGCResult(C.ugc, c->handle, i, &details);
    q.title = clone_string(details.m_rgchTitle);
    q.owner_id = details.m_ulSteamIDOwner;
    q.votes_down = details.m_unVotesDown;
    q.votes_up = details.m_unVotesUp;
    q.file_id = details.m_nPublishedFileId;
    // Get preview image URL for the thumbnail
    char url[256];
    SteamAPI_ISteamUGC_GetQueryUGCPreviewURL(C.ugc, c->handle, i, url,
                                             sizeof(url));
    q.url = clone_string(url);
    q.desc = clone_string(details.m_rgchDescription);
    q.author_name = clone_string(
        SteamFriends()->GetFriendPersonaName(details.m_ulSteamIDOwner));
    arrput(r->items, q);
  }
  SteamAPI_ISteamUGC_ReleaseQueryUGCRequest(C.ugc, c->handle);
  c->handle = k_UGCQueryHandleInvalid;
  return true;
#else
  return false;
#endif
}

void steam_query_call_cancel(void* ctx) {
#ifdef WITH_STEAM
  SteamQueryCall* c = (SteamQueryCall*)ctx;

  if (c->canceled || c->done || c->failed) {
    return;
  }

  if (c->handle != k_UGCQueryHandleInvalid) {
    SteamAPI_ISteamUGC_ReleaseQueryUGCRequest(C.ugc, c->handle);
    c->handle = k_UGCQueryHandleInvalid;
  }

  // Optionally mark as done so callers stop polling
  c->done = true;
  c->failed = false;
  c->canceled = true;
#endif
}

void steam_query_destroy(void* ctx) {
#ifdef WITH_STEAM
  if (!ctx) return;
  SteamQueryCall* c = (SteamQueryCall*)ctx;
  steam_query_call_cancel(c);
  free(c);
#endif
}

void unload_query_results(QueryResult* r) {
#ifdef WITH_STEAM
  if (!r) return;
  for (int i = 0; i < arrlen(r->items); i++) {
    free(r->items[i].title);
    free(r->items[i].url);
    free(r->items[i].desc);
    free(r->items[i].author_name);
  }
  arrfree(r->items);
  *r = QueryResult{};
#endif
}

#ifdef WITH_STEAM
typedef struct {
  HTTPRequestHandle request;
  SteamAPICall_t api_call;
  HTTPRequestCompleted_t result;
  bool done;
  bool failed;
  char url[1024];
} SteamHttpCall;
#endif

void* steam_http_call(const char* url) {
#ifdef WITH_STEAM
  SteamHttpCall* c = (SteamHttpCall*)calloc(1, sizeof(SteamHttpCall));
  snprintf(c->url, sizeof(c->url), "%s", url);
  ISteamHTTP* http = SteamAPI_SteamHTTP();
  c->request =
      SteamAPI_ISteamHTTP_CreateHTTPRequest(http, k_EHTTPMethodGET, url);
  SteamAPI_ISteamHTTP_SendHTTPRequest(http, c->request, &c->api_call);
  return c;
#else
  return NULL;
#endif
}

#ifdef WITH_STEAM
static Image steam_http_get_data_as_image(SteamHttpCall* c, uint32 body_size,
                                          uint8* body) {
  const char* ext = NULL;
  ISteamHTTP* http = SteamAPI_SteamHTTP();
  uint32 header_size = 0;
  if (SteamAPI_ISteamHTTP_GetHTTPResponseHeaderSize(
          http, c->request, "Content-Type", &header_size) &&
      header_size > 0 && header_size < 128) {
    char content_type[128] = {0};
    SteamAPI_ISteamHTTP_GetHTTPResponseHeaderValue(
        http, c->request, "Content-Type", (uint8*)content_type, header_size);
    if (strstr(content_type, "jpeg") || strstr(content_type, "jpg")) {
      ext = ".jpg";
    } else if (strstr(content_type, "png")) {
      ext = ".png";
    }
  }
  if (!ext) return Image{};
  return LoadImageFromMemory(ext, body, (int)body_size);
  return Image{};
}
#endif

bool steam_http_update(void* ctx, Image* img) {
#ifdef WITH_STEAM
  SteamHttpCall* c = (SteamHttpCall*)ctx;

  // Already done or cancelled
  if (c->done || c->request == INVALID_HTTPREQUEST_HANDLE) {
    *img = Image{};
    return true;
  }

  c->done = SteamAPI_ISteamUtils_GetAPICallResult(
      SteamAPI_SteamUtils(), c->api_call, &c->result, sizeof(c->result),
      2101,  // HTTPRequestCompleted_t::k_iCallback
      &c->failed);
  if (!c->done) {
    // pending
    return false;
  }
  if (c->failed || !c->result.m_bRequestSuccessful) {
    SteamAPI_ISteamHTTP_ReleaseHTTPRequest(SteamAPI_SteamHTTP(), c->request);
    c->request = INVALID_HTTPREQUEST_HANDLE;
    *img = Image{};
    return true;
  }

  uint32 body_size = 0;

  ISteamHTTP* http = SteamAPI_SteamHTTP();
  SteamAPI_ISteamHTTP_GetHTTPResponseBodySize(http, c->request, &body_size);

  uint8* body = (uint8*)malloc(body_size);
  SteamAPI_ISteamHTTP_GetHTTPResponseBodyData(http, c->request, body,
                                              body_size);
  *img = steam_http_get_data_as_image(c, body_size, body);
  free(body);
  SteamAPI_ISteamHTTP_ReleaseHTTPRequest(SteamAPI_SteamHTTP(), c->request);
  c->request = INVALID_HTTPREQUEST_HANDLE;
  return true;
#else
  return false;
#endif
}

void steam_http_cancel(void* ctx) {
#ifdef WITH_STEAM
  SteamHttpCall* c = (SteamHttpCall*)ctx;
  if (!c || c->done) return;
  if (c->request != INVALID_HTTPREQUEST_HANDLE) {
    // Unlike UGC, this actually aborts the in-flight request
    SteamAPI_ISteamHTTP_ReleaseHTTPRequest(SteamAPI_SteamHTTP(), c->request);
    c->request = INVALID_HTTPREQUEST_HANDLE;
  }
  c->done = true;
  c->failed = true;
#endif
}

void steam_http_destroy(void* ctx) {
#ifdef WITH_STEAM
  SteamHttpCall* c = (SteamHttpCall*)ctx;
  if (!c) return;
  steam_http_cancel(c);
  free(c);
#endif
}

// const uint8_t* steam_http_get_data(struct SteamHttpCall* c,
//                                    uint32_t* out_size) {
//   if (out_size) *out_size = c->body_size;
//   return c->body;
// }

void steam_subscribe_item(u64 id) {
#ifdef WITH_STEAM
  SteamAPI_ISteamUGC_SubscribeItem(C.ugc, id);
#endif
}

u32 steam_item_state(u64 id) {
#ifdef WITH_STEAM
  int32 state = SteamAPI_ISteamUGC_GetItemState(C.ugc, id);

  u32 out = 0;
  if (state == k_EItemStateNone) {
    // Not subscribed, not known locally
    out = ITEM_STATE_NONE;
  }
  if (state & k_EItemStateSubscribed) {
    // User is subscribed
    out = out | ITEM_STATE_SUBSCRIBED;
  }
  if (state & k_EItemStateInstalled) {
    // Downloaded and installed locally
    out = out | ITEM_STATE_INSTALLED;
  }
  if (state & k_EItemStateDownloading) {
    // Currently downloading
    out = out | ITEM_STATE_DOWNLOADING;
  }
  if (state & k_EItemStateDownloadPending) {
    // Queued for download
    out = out | ITEM_STATE_DOWNLOADPENDING;
  }
  if (state & k_EItemStateNeedsUpdate) {
    // Installed but an update is available
    out = out | ITEM_STATE_NEEDSUPDATE;
  }
  return out;
#else
  return 0;
#endif
}

#ifdef WITH_STEAM
typedef struct {
  uint64 key;
  bool value;
} SteamRequestedEntry;

static SteamRequestedEntry* s_requested = NULL;
#endif

const char* steam_get_author_name(u64 steam_id) {
#ifdef WITH_STEAM
  if (hmgeti(s_requested, steam_id) < 0) {
    SteamAPI_ISteamFriends_RequestUserInformation(SteamAPI_SteamFriends(),
                                                  steam_id, true);
    hmput(s_requested, steam_id, true);
  }
  return SteamAPI_ISteamFriends_GetFriendPersonaName(SteamAPI_SteamFriends(),
                                                     steam_id);
#else
  return NULL;
#endif
}
