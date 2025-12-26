#ifndef CA_GAME_REGISTRY_H
#define CA_GAME_REGISTRY_H
#include <lua.h>

#include "common.h"

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

typedef struct LevelGroup {
  char* id;
  char* name;
  char* desc;
  struct LevelDef** levels;
  int nlevels;
  struct GameRegistry* registry;
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
 * Descriptor of a level, defined by a JSON file.
 */
typedef struct LevelDef {
  sprite_t icon;          /* Icon of the level */
  sprite_t* sprites;      /* Sprites for description */
  int x;                  /* Position in the grid */
  int y;                  /* Position in the grid */
  struct LevelDef** deps; /* dependency levels */
  char* id;               /* Level ID */
  char* name;             /* Level name */
  LevelGroup* group;      /* Level Group  (default=Custom)*/
  char* description;      /* Level description text */
  char** kernels;         /* Absolute path */
  char** assets;   /* Absolute path to assets (PNG only for now, loaded as
                      texture) */
  bool complete;   /* wether level was complete (injected from save)*/
  bool can_choose; /* If can choose level (computed from dependencies) */
  LevelDefExtraItem* extra_content;
} LevelDef;

typedef struct {
  char* key;
  LevelDef* value;
} LevelDefDict;

typedef struct {
  char* key;
  LevelGroup* value;
} LevelGroupDict;

typedef struct GameRegistry {
  TutorialTopic* topics; /* Tutorial stuff */
  LevelGroupDict* groups;
  LevelDefDict* levels;
  LevelGroup** group_order;
  lua_State* L;
} GameRegistry;

GameRegistry* create_game_registry();
void init_mod(GameRegistry* r, const char* mod_path);
LevelDef* get_level_by_id(GameRegistry* r, const char* level_id);
LevelGroup* get_group_by_id(GameRegistry* r, const char* group_id);
void update_levels_completion(GameRegistry* r);
void dispatch_level_complete(LevelDef* ldef);
void save_progress(GameRegistry* r);
void load_progress(GameRegistry* r);
void registry_add_tutorial_topic(GameRegistry* r, const char* topic_id,
                                 const char* name, const char* icon_file);
void registry_add_tutorial_item(GameRegistry* r, const char* topic_id,
                                const char* item_id, const char* name,
                                const char* desc, sprite_t* sprites,
                                const char* icon_file);

TutorialItem* find_wiki_from_id(GameRegistry* r, const char* item_id);

#endif
