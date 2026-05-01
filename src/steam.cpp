#include "steam.h"

#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#include "utils.h"
#include "win_blueprint.h"

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
      if (meta_has_tag(&meta, "blueprint")) {
        add_steam_blueprint(folder, id);
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
    for (int i = 0; i < tags.size(); i++) {
      s_tags.push_back(tags[i].c_str());
    }
    // if (!title.empty()) {
    SteamParamStringArray_t p_tags = {0};
    p_tags.m_ppStrings = s_tags.data();
    p_tags.m_nNumStrings = s_tags.size();
    bool allow_admin_tags = false;
    SteamAPI_ISteamUGC_SetItemTags(C.ugc, handle, &p_tags, allow_admin_tags);
    // SteamAPI_ISteamUGC_RemoveAllItemKeyValueTags(C.ugc, handle);
    // for (int i = 0; i < p.num_kv_tags; i++) {
    //   SteamAPI_ISteamUGC_AddItemKeyValueTag(C.ugc, c->handle, p.kv_keys[i],
    //                                         p.kv_values[i]);
    // }
    SteamAPI_ISteamUGC_SetItemVisibility(
        C.ugc, handle, k_ERemoteStoragePublishedFileVisibilityPublic);
    //    }

    save_steam_metadata(folder.c_str(), C.my_id, C.my_name.c_str(),
                        s_tags.size(), s_tags.data(), 0, NULL, NULL);

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
                        const char* thumb_path, int ntags, const char** tags) {
#ifdef WITH_STEAM
  SteamUploadContext* ctx = new SteamUploadContext();
  if (title) ctx->title = title;
  if (desc) ctx->desc = desc;
  if (chg_notes) ctx->chg_notes = chg_notes;
  if (thumb_path) ctx->thumbnail_path = thumb_path;
  for (int i = 0; i < ntags; i++) {
    ctx->tags.push_back(tags[i]);
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

void steam_load_blueprints() {
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
