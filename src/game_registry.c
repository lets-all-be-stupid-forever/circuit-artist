#include "game_registry.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <msgpack.h>

#include "fs.h"
#include "json.h"
#include "paths.h"
#include "stb_ds.h"
#include "stdlib.h"
#include "steam.h"
#include "ui.h"
#include "utils.h"
#include "wmain.h"

typedef struct {
  GameRegistry* registry;
  const char* mod_root;
} LoadCtx;

/* TODO: needs to be a context */
static LoadCtx _load_ctx = {0};

static int lua_add_group(lua_State* L) {
  GameRegistry* r = _load_ctx.registry;
  const char* root = _load_ctx.mod_root;
  LevelGroup* group = calloc(1, sizeof(LevelDef));
  group->registry = r;

  luaL_checktype(L, 1, LUA_TTABLE);

  lua_getfield(L, 1, "id");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "id field must be a string");
  }
  group->id = clone_string(lua_tostring(L, -1));
  lua_pop(L, 1);  // Remove id from stack

  lua_getfield(L, 1, "name");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "name field must be a string");
  }
  group->name = clone_string(lua_tostring(L, -1));
  lua_pop(L, 1);  // Remove name from stack

  lua_getfield(L, 1, "desc");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "desc field must be a string");
  }
  group->desc = clone_string(lua_tostring(L, -1));
  lua_pop(L, 1);  // Remove desc from stack

  lua_getfield(L, 1, "icon");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "icon field must be a string");
  }
  const char* icon = lua_tostring(L, -1);
  char* icon_path = checkmodpath(root, icon);
  if (!icon_path) {
    return luaL_error(L, "Invalid icon path");
  }
  group->icon = LoadTexture(icon_path);
  free(icon_path);
  lua_pop(L, 1);  // Remove icon from stack

  lua_getfield(L, 1, "deps");
  if (!lua_isnil(L, -1)) {
    luaL_checktype(L, -1, LUA_TTABLE);
    int ndeps = (int)lua_rawlen(L, -1);  // Get array length
    for (int i = 1; i <= ndeps; i++) {
      lua_rawgeti(L, -1, i);
      const char* id = lua_tostring(L, -1);
      LevelGroup* group_dep = get_group_by_id(r, id);
      arrput(group->deps, group_dep);
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  shput(r->groups, group->id, group);
  arrput(r->group_order, group);
  return 0;
}

static void group_add_level(LevelGroup* group, LevelDef* ldef) {
  ldef->group = group;
  group->nlevels++;
  arrput(group->levels, ldef);
}

static int lua_add_level(lua_State* L) {
  LevelDef* ldef = calloc(1, sizeof(LevelDef));
  const char* root = _load_ctx.mod_root;
  GameRegistry* r = _load_ctx.registry;
  // Check that we received a table
  luaL_checktype(L, 1, LUA_TTABLE);

  // Get the "icon" field
  lua_getfield(L, 1, "icon");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "icon field must be a string");
  }
  const char* icon = lua_tostring(L, -1);
  char* icon_path = checkmodpath(root, icon);
  if (!icon_path) {
    return luaL_error(L, "Invalid icon path");
  }
  ldef->icon = create_sprite(LoadTexture(icon_path));
  free(icon_path);
  lua_pop(L, 1);  // Remove icon from stack

  /* Description */
  lua_getfield(L, 1, "description");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "Description field must be a string");
  }
  ldef->description = clone_string(lua_tostring(L, -1));
  load_text_sprites(root, ldef->description, &ldef->sprites);
  lua_pop(L, 1);  // Remove icon from stack

  /* Name */
  lua_getfield(L, 1, "name");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "name field must be a string");
  }
  ldef->name = clone_string(lua_tostring(L, -1));
  lua_pop(L, 1);  // Remove icon from stack

  /* Group */
  lua_getfield(L, 1, "group");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "group field must be a string");
  }
  const char* group_id = lua_tostring(L, -1);
  LevelGroup* group = get_group_by_id(r, group_id);
  if (!group) {
    return luaL_error(L, TextFormat("There's no group with id: %s", group_id));
  }
  lua_pop(L, 1);  // Remove icon from stack

  /* ID */
  lua_getfield(L, 1, "id");
  if (!lua_isstring(L, -1)) {
    return luaL_error(L, "id field must be a string");
  }
  ldef->id = clone_string(lua_tostring(L, -1));
  lua_pop(L, 1);  // Remove icon from stack

  // Get the "content" field (which is a table/array)
  lua_getfield(L, 1, "content");
  if (!lua_istable(L, -1)) {
    return luaL_error(L, "content field must be a table");
  }
  // Iterate through the content array
  int content_length = (int)lua_rawlen(L, -1);  // Get array length
  for (int i = 1; i <= content_length; i++) {
    lua_rawgeti(L, -1, i);  // Get content[i]
    if (lua_isstring(L, -1)) {
      const char* item = lua_tostring(L, -1);
      char* kernel_path = checkmodpath(root, item);
      if (!kernel_path) {
        return luaL_error(L, "Invalid kernel path.");
      }
      arrput(ldef->kernels, kernel_path);
    }
    lua_pop(L, 1);  // Remove item from stack
  }
  lua_pop(L, 1);

  /* Assets */
  lua_getfield(L, 1, "assets");
  if (!lua_istable(L, -1)) {
    return luaL_error(L, "assets field must be a table");
  }
  int assets_length = (int)lua_rawlen(L, -1);  // Get array length
  for (int i = 1; i <= assets_length; i++) {
    lua_rawgeti(L, -1, i);  // Get content[i]
    if (lua_isstring(L, -1)) {
      const char* item = lua_tostring(L, -1);
      char* kernel_path = checkmodpath(root, item);
      if (!kernel_path) {
        return luaL_error(L, "Invalid kernel path.");
      }
      arrput(ldef->assets, kernel_path);
    }
    lua_pop(L, 1);  // Remove item from stack
  }
  lua_pop(L, 1);

  /* Dependencies */
  lua_getfield(L, 1, "deps");
  if (!lua_isnil(L, -1)) {
    luaL_checktype(L, -1, LUA_TTABLE);
    int ndeps = (int)lua_rawlen(L, -1);  // Get array length
    for (int i = 1; i <= ndeps; i++) {
      lua_rawgeti(L, -1, i);
      const char* id = lua_tostring(L, -1);
      assert(id);
      LevelDef* ldef_dep = get_level_by_id(r, id);
      arrput(ldef->deps, ldef_dep);
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  /* Extra Text */
  lua_getfield(L, 1, "extra_text");
  if (!lua_isnil(L, -1)) {
    luaL_checktype(L, -1, LUA_TTABLE);
    int nextra = (int)lua_rawlen(L, -1);  // Get array length
    for (int i = 1; i <= nextra; i++) {
      lua_rawgeti(L, -1, i);
      if (lua_istable(L, -1)) {
        LevelDefExtraItem item = {0};

        // Check if this is an image item
        lua_getfield(L, -1, "img");
        if (lua_isstring(L, -1)) {
          // Image item - load texture
          const char* img_path = lua_tostring(L, -1);
          char* full_path = checkmodpath(root, img_path);
          if (!full_path) {
            lua_pop(L, 1);
            return luaL_error(L, "Invalid image path in extra_text");
          }
          item.tex = LoadTexture(full_path);
          free(full_path);
          lua_pop(L, 1);  // Pop img field

          // Check for optional title field
          lua_getfield(L, -1, "title");
          if (lua_isstring(L, -1)) {
            item.title = clone_string(lua_tostring(L, -1));
          } else {
            item.title = NULL;
          }
          lua_pop(L, 1);  // Pop title field

          // Check for optional scale field
          lua_getfield(L, -1, "scale");
          if (lua_isnumber(L, -1)) {
            item.scale = (int)lua_tointeger(L, -1);
          } else {
            item.scale = 1;  // Default scale
          }
          lua_pop(L, 1);  // Pop scale field

          item.text = NULL;
          item.wiki = NULL;
        } else {
          lua_pop(L, 1);  // Pop nil img field

          // Check if this is a wiki item
          lua_getfield(L, -1, "wiki");
          if (lua_isstring(L, -1)) {
            // Wiki item - set wiki field, leave title and text as NULL
            item.wiki = clone_string(lua_tostring(L, -1));
            item.title = NULL;
            item.text = NULL;
            lua_pop(L, 1);  // Pop wiki field
          } else {
            // Not a wiki item - pop wiki field first, then get title and text
            lua_pop(L, 1);  // Pop nil wiki field

            // Get title field
            lua_getfield(L, -1, "title");
            if (lua_isstring(L, -1)) {
              item.title = clone_string(lua_tostring(L, -1));
            }
            lua_pop(L, 1);

            // Get text field
            lua_getfield(L, -1, "text");
            if (lua_isstring(L, -1)) {
              item.text = clone_string(lua_tostring(L, -1));
            }
            lua_pop(L, 1);

            item.wiki = NULL;
          }
        }

        arrput(ldef->extra_content, item);
      }
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  shput(r->levels, ldef->id, ldef);
  group_add_level(group, ldef);
  printf("ADDED %s->%s\n", ldef->group->id, ldef->id);
  return 0;
}

static int lua_loadtxt(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  if (!path) {
    return luaL_error(L, "Expected a string path");
  }
  char* apath = checkmodpath(_load_ctx.mod_root, path);
  if (!apath) abort();
  char* content = LoadFileText(apath);
  free(apath);
  lua_pushlstring(L, content, strlen(content));
  UnloadFileText(content);
  return 1;
}

void init_mod(GameRegistry* r, const char* mod_asset_path) {
  _load_ctx.registry = r;
  assert(!_load_ctx.mod_root);
  char* mod_path = get_asset_path(mod_asset_path);
  _load_ctx.mod_root = mod_path;
  // TODO: do an extra safe check that mod_path is within acceptable game paths
  char* path = checkmodpath(mod_path, "main.lua");
  if (dofile_with_traceback(r->L, path) != LUA_OK) {
    // TODO: What do I do here?
    // ui_handle_lua_error(L);
    free(path);
    free(mod_path);
    return;
  }
  free(path);
  free(mod_path);
  _load_ctx.mod_root = NULL;
  _load_ctx.registry = NULL;
}

LevelDef* get_level_by_id(GameRegistry* r, const char* level_id) {
  return shget(r->levels, level_id);
}

// void handle_lua_error(lua_State* L) {
//   C.lua_error = clone_string(lua_tostring(L, -1));
//   lua_close(L);
// }

static int lua_tutorial_add_topic(lua_State* L) {
  GameRegistry* r = _load_ctx.registry;
  const char* root = _load_ctx.mod_root;
  const char* id = luaL_checkstring(L, 1);
  const char* name = luaL_checkstring(L, 2);
  const char* icon = luaL_checkstring(L, 3);
  char* icon2 = checkmodpath(root, icon);
  printf(" ------> ICON2=%s\n", icon2);
  registry_add_tutorial_topic(r, id, name, icon2);
  free(icon2);
  return 0;
}

static int lua_tutorial_add_item(lua_State* L) {
  GameRegistry* r = _load_ctx.registry;
  const char* root = _load_ctx.mod_root;
  const char* topic_id = luaL_checkstring(L, 1);
  const char* id = luaL_checkstring(L, 2);
  const char* name = luaL_checkstring(L, 3);
  const char* desc = luaL_checkstring(L, 4);
  const char* icon = luaL_checkstring(L, 5);
  char* icon_path = checkmodpath(root, icon);
  sprite_t* sprites;
  load_text_sprites(root, desc, &sprites);
  registry_add_tutorial_item(r, topic_id, id, name, desc, sprites, icon_path);
  free(icon_path);
  return 0;
}

GameRegistry* create_game_registry() {
  GameRegistry* r = calloc(1, sizeof(GameRegistry));
  lua_State* L = luaL_newstate();
  if (!L) {
    // fprintf(stderr, "Failed to create Lua state\n");
    // TODO: Habe a proper crash screen
    ui_crash("Failed to create Lua state");
    return NULL;
  }
  luaL_openlibs(L);
  lua_register(L, "add_group", lua_add_group);
  lua_register(L, "add_level", lua_add_level);
  lua_register(L, "tut_add_topic", lua_tutorial_add_topic);
  lua_register(L, "tut_add_item", lua_tutorial_add_item);
  lua_register(L, "loadtxt", lua_loadtxt);
  r->L = L;
  return r;
}

LevelGroup* get_group_by_id(GameRegistry* r, const char* group_id) {
  return shget(r->groups, group_id);
}

#ifdef WITH_STEAM
void steam_stat_sync(GameRegistry* r) {
  if (!is_steam_on()) return;
  int ng = arrlen(r->group_order);
  bool dirty = false;
  for (int ig = 0; ig < ng; ig++) {
    LevelGroup* g = r->group_order[ig];
    int nl = arrlen(g->levels);
    int comp_here = 0;
    for (int il = 0; il < nl; il++) {
      if (g->levels[il]->complete) {
        comp_here++;
      }
    }
    const char* name = TextFormat("%s_progress", g->id);
    int comp_there = steam_get_stats(name);
    /* Don't update if the local progress is inferior. */
    if (comp_there < comp_here) {
      steam_set_stats(name, comp_here);
      printf("update_steam_stat: %s  %d --> %d \n", name, comp_there,
             comp_here);
      dirty = true;
    }
  }

  if (dirty) {
    steam_store_stats();
  }
}
#endif

void update_levels_completion(GameRegistry* r) {
  int ng = arrlen(r->group_order);
  for (int ig = 0; ig < ng; ig++) {
    LevelGroup* g = r->group_order[ig];
    g->can_choose = true;
    g->complete = true;
    if (g->deps) {
      g->can_choose = true;
      int ndep = arrlen(g->deps);
      for (int idep = 0; idep < ndep; idep++) {
        if (!g->deps[idep]->complete) {
          g->can_choose = false;
          break;
        }
      }
    }

    int nl = arrlen(g->levels);
    for (int il = 0; il < nl; il++) {
      LevelDef* ldef = g->levels[il];
      ldef->can_choose = g->can_choose;
      if (!ldef->complete) g->complete = false;
      if (ldef->deps) {
        int ndep = arrlen(ldef->deps);
        for (int idep = 0; idep < ndep; idep++) {
          if (!ldef->deps[idep]->complete) {
            // ldef->can_choose = false; /* Uncomment here to enable
            // can_choose*/
            break;
          }
        }
      }
    }
  }
#ifdef WITH_STEAM
  steam_stat_sync(r);
#endif
}

void dispatch_level_complete(LevelDef* ldef) {
  if (!ldef->complete) {
    ldef->complete = true;
    play_sound(SOUND_LEVEL_COMPLETE);
    update_levels_completion(ldef->group->registry);
    save_progress(ldef->group->registry);
  }
}

void save_progress(GameRegistry* r) {
  json_object *items, *item, *str;
  json_object* complete;
  json_object* levels;
  json_object* root = json_object_new_object();
  if (!root) {
    ui_crash("Couldn't create save file.");
    return;
  }
  json_object_object_add(root, "version", json_object_new_int(0));

  levels = json_object_new_object();
  int nl = shlen(r->levels);
  for (int i = 0; i < nl; i++) {
    complete = json_object_new_boolean(r->levels[i].value->complete);
    json_object_object_add(levels, r->levels[i].value->id, complete);
  }
  json_object_object_add(root, "levels", levels);
  if (json_object_to_file_ext(get_progress_path(), root,
                              JSON_C_TO_STRING_PRETTY) < 0) {
    fprintf(stderr, "Error writing to file\n");
    json_object_put(root);
    ui_crash("Error creating save file\n");
    return;
  }
  json_object_put(root);
}

void load_progress(GameRegistry* r) {
  if (!FileExists(get_progress_path())) {
    return;
  }

  json_object* levels;
  json_object* complete;
  json_object* root = json_object_from_file(get_progress_path());
  assert(root);
  if (json_object_object_get_ex(root, "levels", &levels)) {
    int nl = shlen(r->levels);
    for (int i = 0; i < nl; i++) {
      LevelDef* ldef = r->levels[i].value;
      if (json_object_object_get_ex(levels, ldef->id, &complete)) {
        ldef->complete = json_object_get_boolean(complete);
      }
    }
  }
  json_object_put(root);
  update_levels_completion(r);
}

static int find_topic_by_id(GameRegistry* r, const char* topic_id) {
  TutorialTopic* topics = r->topics;
  for (int i = 0; i < arrlen(topics); i++) {
    if (strcmp(topics[i].id, topic_id) == 0) {
      return i;
    }
  }
  return -1;
}

void registry_add_tutorial_topic(GameRegistry* r, const char* topic_id,
                                 const char* name, const char* icon_path) {
  assert(FileExists(icon_path));
  int idx = find_topic_by_id(r, topic_id);
  if (idx != -1) {
    // TODO Handle this error properly
    abort();
  }

  TutorialTopic topic = {0};
  topic.id = clone_string(topic_id);
  topic.name = clone_string(name);
  topic.icon = create_sprite(LoadTexture(icon_path));
  arrput(r->topics, topic);
}

void registry_add_tutorial_item(GameRegistry* r, const char* topic_id,
                                const char* item_id, const char* name,
                                const char* desc, sprite_t* sprites,
                                const char* icon_path) {
  assert(FileExists(icon_path));
  int idx = find_topic_by_id(r, topic_id);
  TutorialTopic* topic = &r->topics[idx];
  TutorialItem item = {0};
  item.icon = create_sprite(LoadTexture(icon_path));
  item.desc = clone_string(desc);
  item.desc_imgs = sprites;
  item.id = clone_string(item_id);
  item.name = clone_string(name);
  arrput(topic->items, item);
}

TutorialItem* find_wiki_from_id(GameRegistry* r, const char* item_id) {
  int nt = arrlen(r->topics);
  for (int it = 0; it < nt; it++) {
    int ni = arrlen(r->topics[it].items);
    for (int ii = 0; ii < ni; ii++) {
      TutorialItem* item = &r->topics[it].items[ii];
      if (strcmp(r->topics[it].items[ii].id, item_id) == 0) {
        return item;
      }
    }
  }
  return NULL;
}
