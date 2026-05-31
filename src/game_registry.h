#ifndef CA_GAME_REGISTRY_H
#define CA_GAME_REGISTRY_H
#include <lua.h>

#include "blueprint.h"
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
  bool complete; /* A level is complete when all levels are complete */
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
 * Descriptor of a level.
 *
 * Each level id is defined by:
 *   campaign:<id>      --> Campaign levels
 *   steam:<id>         --> Workshop subscribed levels
 *   local:<folder>     --> Levels in custom_levels/ folder.
 *   ca_custom:<folder> --> for default levels shipped by CA.
 */
typedef struct LevelDef {
  /* Common Level stuff */
  char* id;          /* Level ID */
  char* name;        /* Level name */
  char* description; /* Level description text */
  sprite_t* sprites; /* Sprites for description */
  char* kernel;      /* Absolute path to the kernel script */
  char* folder;      /* Root folder */

  /* Level type */
  bool is_campaign;
  bool is_custom_local;
  bool is_custom_official;
  bool is_custom_steam;

  /* Campaign Level stuff */
  bool complete;   /* wether level was complete (injected from save)*/
  bool can_choose; /* If can choose level (computed from dependencies) */
  LevelDefExtraItem* extra_content;
  struct LevelDef** deps; /* dependency levels */
  LevelGroup* group;      /* Level Group  (default=Custom)*/

  /* Custom Level stuff */
  char* steam_author; /* Author name generated during workshop upload */
  bool unsubscribed;
} LevelDef;

typedef struct {
  bool simu_neon;
  bool simu_sound;
  bool paint_sound;
  bool always_on_top;
} GameConfig;

typedef struct {
  char* key;
  LevelDef* value;
} LevelDefDict;

typedef struct {
  char* key;
  LevelGroup* value;
} LevelGroupDict;

typedef struct GameRegistry {
  GameConfig cfg;
  TutorialTopic* topics; /* Tutorial stuff */
  LevelGroupDict* groups;
  LevelDefDict* levels;
  LevelGroup** group_order;
  Mod** mods;
  lua_State* L;
  BlueprintStore store;

  /* Custom Levels */
  LevelDef** workshop_custom_levels;
  LevelDef** local_custom_levels;
  LevelDef** official_custom_levels;
} GameRegistry;

void init_game_registry();
GameRegistry* getreg();
LevelDef* get_level_by_id(const char* level_id);
LevelGroup* get_group_by_id(const char* group_id);
void dispatch_level_complete(LevelDef* ldef);
void save_progress();
void load_progress();
void add_steam_level_from_folder(const char* folder, u64 steam_id);
TutorialItem* find_wiki_from_id(const char* item_id);
u64 extract_item_from_id(const char* id);
char* get_custom_levels_folder();
bool folder_is_level(const char* folder);

bool is_paint_sound_on();
bool is_circuit_sound_on();
bool is_circuit_neon_on();
bool is_always_on_top();
void on_always_on_top_change();

const char* get_level_name_by_id(const char* id);

#if defined(__cplusplus)
}
#endif

#endif
