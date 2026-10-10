#include "i18n.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paths.h"
#include "stb_ds.h"
#include "utils.h"

static struct {
  struct {
    char* key;
    char* value;
  }* strings;
} C = {0};

const char* TR(const char* key) {
  char* v = shget(C.strings, key);
  return v ? v : key;
}

/* this function registers the token+value */
void TR_add(const char* key, const char* value) {
  shput(C.strings, clone_string(key), clone_string(value));
}

struct tr_s T = {0};

static void check_unused_keys(lua_State* L, const char* name, bool verbose) {
  if (!verbose) return;

  static const char* known[] = {
#define X(s) #s,
      TR_FIELDS(X)
#undef X
          NULL};

  lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return;
  }

  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TSTRING) {
      const char* key = lua_tostring(L, -2);
      if (key[0] != '_') {
        bool found = false;
        for (int i = 0; known[i]; i++) {
          if (strcmp(key, known[i]) == 0) {
            found = true;
            break;
          }
        }
        // if (!found) printf("WARNING i18n(%s): unused key '%s'\n", name, key);
      }
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

void load_locale(const char* name, bool verbose) {
  char rel[64];
  snprintf(rel, sizeof(rel), "locale/%s.lua", name);
  char* path = get_asset_path(rel);

  lua_State* L = luaL_newstate();

  if (luaL_loadfile(L, path) != LUA_OK || lua_pcall(L, 0, 0, 0) != LUA_OK) {
    if (verbose)
      printf("WARNING i18n: failed to load locale '%s': %s\n", name,
             lua_tostring(L, -1));
    lua_close(L);
    free(path);
    return;
  }

  free(path);

#define X(s)                                                   \
  lua_getglobal(L, #s);                                        \
  if (lua_isstring(L, -1))                                     \
    TR_add(#s, lua_tostring(L, -1));                           \
  else if (verbose)                                            \
    printf("WARNING i18n(%s): missing key '%s'\n", name, #s); \
  lua_pop(L, 1);
  TR_FIELDS(X)
#undef X

  /* Register all remaining global strings and string fields in table globals.
   * This covers keys not in TR_FIELDS (e.g. T.xxx entries in mod locale files). */
  lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    if (lua_type(L, -2) == LUA_TSTRING) {
      const char* key = lua_tostring(L, -2);
      if (key[0] != '_') {
        if (lua_type(L, -1) == LUA_TSTRING) {
          TR_add(key, lua_tostring(L, -1));
        } else if (lua_type(L, -1) == LUA_TTABLE) {
          lua_pushnil(L);
          while (lua_next(L, -2) != 0) {
            if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TSTRING)
              TR_add(lua_tostring(L, -2), lua_tostring(L, -1));
            lua_pop(L, 1);
          }
        }
      }
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  check_unused_keys(L, name, verbose);

  lua_close(L);
}

void init_i18n() {
  load_locale("en", true);

#define X(s) T.s = TR(#s);
  TR_FIELDS(X)
#undef X
}

static int lua_TR_fn(lua_State* L) {
  const char* key = luaL_checkstring(L, 1);
  lua_pushstring(L, TR(key));
  return 1;
}

void i18n_register_lua(lua_State* L) { lua_register(L, "TR", lua_TR_fn); }
