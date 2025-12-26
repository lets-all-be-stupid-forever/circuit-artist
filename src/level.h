#ifndef CA_LEVEL_H
#define CA_LEVEL_H
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <msgpack.h>

#include "buffer.h"
#include "common.h"
#include "game_registry.h"
#include "pin_spec.h"
#include "stdbool.h"

struct Sim;

typedef struct {
  Texture2D tex;
} LevelAsset;

/*
 * Each level is defined by a lua script.
 * The level is self-contained configurations and everything.
 * The level is re-loaded every time the
 *
 * Every path must:
 *    (i) Be a valid path from the root directory
 *    In content, paths must be referenced somehow :( (maybe add a key/val
 * thing?) So, given root and path, the abs path is only valid if it's in the
 * folder
 */
typedef struct {
  PinGroup* pg;   /* Circuit interface */
  LevelDef* ldef; /* Paths */
  bool error;
  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  struct Sim* sim;
  LevelAsset* assets;
  bool lua_error;
  lua_State* L;
} Level;

bool level_init(Level* lvl, LevelDef* ldef);
void level_destroy(Level* lvl);
void level_draw(Level* lvl);
void level_draw_board(Level* lvl);

void level_lua_start(Level* lvl);
void level_lua_update(Level* lvl, Buffer* buffer);
void level_lua_fwbw(Level* lvl, bool fw, Buffer buf);

Buffer lua_tobuffer(lua_State* L, int idx);
int lua_frombuffer(lua_State* L, Buffer buf);

#endif
