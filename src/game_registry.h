#ifndef CA_GAME_REGISTRY_H
#define CA_GAME_REGISTRY_H
#include <lua.h>

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct LevelGroup;
struct LevelDef;
struct GameRegistry;

typedef struct {
  char* name;
  char* desc;
  char* id;
  sprite_t* desc_imgs;
  sprite_t icon;
} TutorialItem;

typedef struct {
  char* name;
  char* id;
  sprite_t icon;
  TutorialItem* items;
} TutorialTopic;

typedef struct Mod {
  char* root;       /* Absolute path to the mod's root folder */
  bool local;       /* Flag for local mods */
  bool default_mod; /* Flag for default mod (readonly) */
} Mod;

typedef struct LevelGroup {
  char* id;
  char* name;
  char* desc;
  struct LevelDef** levels;
  int nlevels;
  struct GameRegistry* registry;
  Mod* mod;
  bool allow_inter_mod; /* Allow levels from other mods to be added to this
                           group */
  bool complete;        /* A level is complete when all levels are complete */
  struct LevelGroup** deps;
  bool can_choose;
  Texture icon;
} LevelGroup;

typedef struct {
  char* title;
  char* text;
  char* wiki;
  Texture tex;
  int scale;
} LevelDefExtraItem;

/*
 * Descriptor of a level, defined by a JSON file.
 */
typedef struct LevelDef {
  char* id;          /* Level ID */
  char* name;        /* Level name */
  char* description; /* Level description text */
  sprite_t* sprites; /* Sprites for description */
  char* kernel;      /* Absolute path to the kernel script */
  bool complete;     /* wether level was complete (injected from save)*/
  bool can_choose;   /* If can choose level (computed from dependencies) */
  LevelDefExtraItem* extra_content;
  struct LevelDef** deps; /* dependency levels */
  LevelGroup* group;      /* Level Group  (default=Custom)*/
  Mod* mod;               /* The mod this level belongs to */
} LevelDef;

typedef struct {
  char* key;
  LevelDef* value;
} LevelDefDict;

typedef struct {
  char* key;
  LevelGroup* value;
} LevelGroupDict;

typedef enum {
  CUSTOM_LEVEL_UNK = 0,
  CUSTOM_LEVEL_LOCAL,
  CUSTOM_LEVEL_STEAM,
  CUSTOM_LEVEL_OFFICIAL,
} CustomLevelType;

/* A custom level entry.
 * Each level id is defined by:
 *   steam:<id> --> for workshop subscribed levels
 *   local:<folder> --> for levels in custom_levels/ folder.
 *   ca_custom:<folder> --> for default levels shipped by CA.
 **/
typedef struct {
  char* id;   /* runtime id of the level */
  char* name; /* Level name */
  char* desc; /* Level description */
  char* folder;
  char* steam_author;   /* Author name generated during workshop upload */
  sprite_t* desc_imgs;  /* Level images */
  CustomLevelType type; /*1 = official, 2=local 3=workshp*/
  bool unsubscribed;
} CustomLevelDef;

typedef struct GameRegistry {
  TutorialTopic* topics; /* Tutorial stuff */
  LevelGroupDict* groups;
  LevelDefDict* levels;
  LevelGroup** group_order;
  Mod** mods;
  lua_State* L;

  /* Custom Levels */
  CustomLevelDef** workshop_custom_levels;
  CustomLevelDef** local_custom_levels;
  CustomLevelDef** official_custom_levels;
} GameRegistry;

GameRegistry* create_game_registry();
void init_mods(GameRegistry* r);
LevelDef* get_level_by_id(GameRegistry* r, const char* level_id);
LevelGroup* get_group_by_id(GameRegistry* r, const char* group_id);
void dispatch_level_complete(LevelDef* ldef);
void save_progress(GameRegistry* r);
void load_progress(GameRegistry* r);
void registry_add_tutorial_topic(GameRegistry* r, const char* topic_id,
                                 const char* name, const char* icon_file);
void registry_add_tutorial_item(GameRegistry* r, const char* topic_id,
                                const char* item_id, const char* name,
                                const char* desc, sprite_t* sprites,
                                const char* icon_file);
void add_steam_level_from_folder(GameRegistry* r, const char* folder,
                                 u64 steam_id);
TutorialItem* find_wiki_from_id(GameRegistry* r, const char* item_id);
u64 extract_item_from_id(const char* id);
char* get_custom_levels_folder();
const char* get_custom_level_kernel_path(CustomLevelDef* ldef);
bool folder_is_level(const char* folder);

#if defined(__cplusplus)
}
#endif

#endif
