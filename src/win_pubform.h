#ifndef CA_WIN_PUBFORM_H
#define CA_WIN_PUBFORM_H
#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
  STEAM_ITEM_BLUEPRINT,
  STEAM_ITEM_LEVEL,
} SteamItemType;

typedef struct {
  SteamItemType type;
  const char* folder;
  const char* default_title;
  const char* default_desc;
  const char* default_thumbnail_path;
  int num_tags;
  const char** tags;
} PubformParams;

void win_pubform_init();
void win_pubform_open(PubformParams params);
void win_pubform_update();
void win_pubform_draw();
void win_pubform_on_close();
#if defined(__cplusplus)
}
#endif

#endif
