#include "lua_level.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <msgpack.h>

#include "assert.h"
#include "font.h"
#include "fs.h"
#include "json.h"
#include "log.h"
#include "raylib.h"
#include "rlgl.h"
#include "sim.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"
#include "utils.h"
#include "widgets.h"

#define NUM_BUTTONS 42

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
  Sim* sim;
  LevelAsset* assets;
  lua_State* L;
} LuaLevel;

static inline Status status_lua_error(lua_State* L) {
  return status_error(lua_tostring(L, -1));
}

// Recursive function to pack Lua objects to MessagePack
static void lua_to_msgpack(lua_State* L, int index, msgpack_packer* pk) {
  index = lua_absindex(L, index);  // Handle relative indices

  int type = lua_type(L, index);

  switch (type) {
    case LUA_TNIL:
      msgpack_pack_nil(pk);
      break;

    case LUA_TBOOLEAN:
      if (lua_toboolean(L, index))
        msgpack_pack_true(pk);
      else
        msgpack_pack_false(pk);
      break;

    case LUA_TNUMBER:
      if (lua_isinteger(L, index))
        msgpack_pack_int64(pk, lua_tointeger(L, index));
      else
        msgpack_pack_double(pk, lua_tonumber(L, index));
      break;

    case LUA_TSTRING: {
      size_t len;
      const char* str = lua_tolstring(L, index, &len);
      msgpack_pack_str(pk, len);
      msgpack_pack_str_body(pk, str, len);
      break;
    }

    case LUA_TTABLE: {
      // Determine if it's an array or map
      size_t array_size = lua_rawlen(L, index);

      if (array_size > 0) {
        // It's an array
        msgpack_pack_array(pk, array_size);
        for (size_t i = 1; i <= array_size; i++) {
          lua_rawgeti(L, index, i);
          lua_to_msgpack(L, -1, pk);
          lua_pop(L, 1);
        }
      } else {
        // It's a map - count entries first
        size_t count = 0;
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
          count++;
          lua_pop(L, 1);
        }

        msgpack_pack_map(pk, count);
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
          // Serialize key
          lua_to_msgpack(L, -2, pk);
          // Serialize value
          lua_to_msgpack(L, -1, pk);
          lua_pop(L, 1);
        }
      }
      break;
    }

    default:
      // Unsupported type - pack as nil or error
      msgpack_pack_nil(pk);
      break;
  }
}

/*
 * Uses old algorithm for defining pins, with the side image generation.
 * The plan is to replace, in further versions, with a more powerful API where
 * users can manually create the API image and define pin locations.
 *
 * The spec is:
 *
 * ports = {
 *  {name="Ain", width=4, input=true},
 *  {name="Bin", width=4, input=false},
 * }
 *
 */
static void lua_level_init_pins(LuaLevel* lvl, LevelAPI* api) {
  lua_State* L = lvl->L;
  lua_getglobal(L, "PORTS");
  assert(lua_istable(L, -1));
#if 0
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return NULL;
  }
#endif
  int nport = lua_rawlen(L, -1);
  PinGroup* out = NULL;
  for (int i = 0; i < nport; i++) {
    PinGroup pg = {0};
    lua_rawgeti(L, -1, i + 1);
    lua_getfield(L, -1, "name");
    pg.id = clone_string(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "width");
    int width = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "input");
    pg.type = lua_toboolean(L, -1) ? PIN_IMG2LUA : PIN_LUA2IMG;
    lua_pop(L, 1);

    lua_getfield(L, -1, "pins");
    for (int j = 0; j < width; j++) {
      lua_rawgeti(L, -1, j + 1);
      /* pin = {x=0,y=2}*/
      lua_getfield(L, -1, "x");
      int x = lua_tointeger(L, -1);
      lua_pop(L, 1);
      lua_getfield(L, -1, "y");
      int y = lua_tointeger(L, -1);
      lua_pop(L, 1);
      lua_pop(L, 1);
      pg_add_pin(&pg, x, y);
    }
    lua_pop(L, 1);
    lua_pop(L, 1);
    level_api_add_pg(api, pg);
  }
  lua_pop(L, 1);
}

static LuaLevel* lua_getlevel(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "level_ptr");
  LuaLevel* lvl = (LuaLevel*)lua_touserdata(L, -1);
  lua_pop(L, 1);
  return lvl;
}

#if 0
/* Retrieves the Sim* object from lua registry */
static Sim* lua_getsim(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "sim_ptr");
  Sim* sim = (Sim*)lua_touserdata(L, -1);
  lua_pop(L, 1);
  return sim;
}
#endif

static Sim* lua_getsim(lua_State* L) { return lua_getlevel(L)->sim; }

int read_table_floats(lua_State* L, int idx, int n, float* values) {
  luaL_checktype(L, idx, LUA_TTABLE);
  for (int i = 0; i < n; i++) {
    lua_rawgeti(L, idx,
                i + 1);  // Push table[i+1] onto stack (Lua is 1-indexed)
    if (!lua_isnumber(L, -1)) {
      return luaL_error(L, "element %d of argument 2 is not a number", i + 1);
    }
    values[i] = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);  // Remove the value from stack
  }
  return 0;
};

static int lua_draw_rectangle_pro(lua_State* L) {
  LuaLevel* lvl = lua_getlevel(L);
  Color c;
  Rectangle rec;
  Vector2 orig;
  float rot;
  int i = 1;
  rec.x = (float)luaL_checknumber(L, i++);
  rec.y = (float)luaL_checknumber(L, i++);
  rec.width = (float)luaL_checknumber(L, i++);
  rec.height = (float)luaL_checknumber(L, i++);
  orig.x = (float)luaL_checknumber(L, i++);
  orig.y = (float)luaL_checknumber(L, i++);
  rot = (float)luaL_checknumber(L, i++);
  c.r = (int)luaL_checknumber(L, i++);
  c.g = (int)luaL_checknumber(L, i++);
  c.b = (int)luaL_checknumber(L, i++);
  c.a = (int)luaL_checknumber(L, i++);
  DrawRectanglePro(rec, orig, rot, c);
  return 0;
}

static int lua_draw_texture_pro(lua_State* L) {
  LuaLevel* lvl = lua_getlevel(L);
  int iasset = luaL_checkinteger(L, 1);
  if (iasset < 0 || iasset >= arrlen(lvl->assets)) {
    return luaL_error(L, "invalid asset index %d (num assets: %d)", iasset,
                      arrlen(lvl->assets));
  }
  Rectangle src;
  Rectangle dst;
  Vector2 orig;
  float rot;
  int i = 2;
  Color c;
  src.x = (float)luaL_checknumber(L, i++);
  src.y = (float)luaL_checknumber(L, i++);
  src.width = (float)luaL_checknumber(L, i++);
  src.height = (float)luaL_checknumber(L, i++);
  dst.x = (float)luaL_checknumber(L, i++);
  dst.y = (float)luaL_checknumber(L, i++);
  dst.width = (float)luaL_checknumber(L, i++);
  dst.height = (float)luaL_checknumber(L, i++);
  orig.x = (float)luaL_checknumber(L, i++);
  orig.y = (float)luaL_checknumber(L, i++);
  rot = (float)luaL_checknumber(L, i++);
  c.r = (int)luaL_checknumber(L, i++);
  c.g = (int)luaL_checknumber(L, i++);
  c.b = (int)luaL_checknumber(L, i++);
  c.a = (int)luaL_checknumber(L, i++);
  Texture tex = lvl->assets[iasset].tex;
  DrawTexturePro(tex, src, dst, orig, rot, c);
  return 0;
}

#define getnwire(s) (s->wg.nwire)
#define isskt(s) (s == PIN_IMG2LUA)
#define isdrv(s) (s == PIN_LUA2IMG)

static int lua_pget(lua_State* L) {
  i64 iport = luaL_checkinteger(L, 1);
  Sim* sim = lua_getsim(L);
  int npin = arrlen(sim->api->pg);
  if (iport >= npin) {
    return luaL_error(
        L, TextFormat("Invalid Pin Number: %d (max %d)", iport, npin - 1));
  }
  if (!isskt(sim->api->pg[iport].type)) {
    return luaL_error(
        L, "Trying to read pin (%d) that is not a socket: can't read", iport);
  }

  PinComm pc = sim_port_read(sim, iport);
  i64 r = pc.b;
  lua_pushinteger(L, pc.b);
  return 1;
}

static int lua_notify_level_complete(lua_State* L) {
  LuaLevel* lvl = lua_getlevel(L);
  Sim* sim = lvl->sim;
  dispatch_level_complete(lvl->ldef);
  sim->level_complete_dispatched_at = sim->state.cur_tick;
  return 0;
}

static int lua_pset(lua_State* L) {
  i64 iport = luaL_checkinteger(L, 1);
  i64 b = luaL_checkinteger(L, 2);
  PinComm pc = {.b = b, .f = 0};
  Sim* sim = lua_getsim(L);
  sim_port_write(sim, iport, pc);
  return 0;
}

int lua_print_redirect(lua_State* L) {
  int n = lua_gettop(L);
  lua_getglobal(L, "tostring");

  for (int i = 1; i <= n; i++) {
    lua_pushvalue(L, -1);
    lua_pushvalue(L, i);
    lua_call(L, 1, 1);

    const char* s = lua_tostring(L, -1);
    if (s) lua_log_text(s);

    lua_pop(L, 1);
  }
  return 0;
}

int l_print(lua_State* L) {
  int nargs = lua_gettop(L);  // Get number of arguments

  // First argument (required): text
  if (nargs < 1) {
    return luaL_error(L, "print requires at least 1 argument (text)");
  }
  const char* text = luaL_checkstring(L, 1);

  // Second argument (optional): x
  int has_x = (nargs >= 2 && !lua_isnil(L, 2));
  int x = has_x ? luaL_checkinteger(L, 2) : 0;  // Default to 0 if not provided

  // Third argument (optional): y
  int has_y = (nargs >= 3 && !lua_isnil(L, 3));
  int y = has_y ? luaL_checkinteger(L, 3) : 0;

  // Third argument (optional): y
  int has_c = (nargs >= 4 && !lua_isnil(L, 4));
  int c = has_c ? luaL_checkinteger(L, 4) : 1;

  Color lut[] = {
      BLACK,
      WHITE,
      BLANK,
  };
  int w = get_rendered_text_size(text).x;
  if (c != 2) {
    font_draw_texture(text, x, y, lut[c]);
  }
  lua_pushinteger(L, w);
  return 1;
}

int lua_rl_translatef(lua_State* L) {
  int i = 1;
  int x = (float)luaL_checknumber(L, i++);
  int y = (float)luaL_checknumber(L, i++);
  int z = (float)luaL_checknumber(L, i++);
  rlTranslatef(x, y, z);
  return 0;
}

int lua_rl_pop_matrix(lua_State* L) {
  rlPopMatrix();
  return 0;
}

int lua_rl_push_matrix(lua_State* L) {
  rlPushMatrix();
  return 0;
}

int lua_rl_scalef(lua_State* L) {
  int i = 1;
  int x = (float)luaL_checknumber(L, i++);
  int y = (float)luaL_checknumber(L, i++);
  int z = (float)luaL_checknumber(L, i++);
  rlScalef(x, y, z);
  return 0;
}

int lua_measure_text_size(lua_State* L) {
  const char* text = luaL_checkstring(L, 1);
  int x = get_rendered_text_size(text).x;
  lua_pushinteger(L, x);
  return 1;
}

int lua_draw_box(lua_State* L) {
  int nargs = lua_gettop(L);  // Get number of arguments

  // First argument (required): text
  if (nargs < 1) {
    return luaL_error(L, "print requires at least 1 argument (text)");
  }
  const char* text = luaL_checkstring(L, 1);
  int i = 2;
  int x = (float)luaL_checknumber(L, i++);
  int y = (float)luaL_checknumber(L, i++);
  int w = (float)luaL_checknumber(L, i++);
  int r = (float)luaL_checknumber(L, i++);
  int g = (float)luaL_checknumber(L, i++);
  int b = (float)luaL_checknumber(L, i++);
  int a = (float)luaL_checknumber(L, i++);
  // font_draw_texture(text, x, y, (Color){r, g, b, a});
  Rectangle rect = {
      x,
      y,
      w,
      0,
  };
  Color c = (Color){r, g, b, a};
  draw_text_box_advanced(text, rect, c, NULL, NULL);
  return 0;
}

int lua_draw_font(lua_State* L) {
  int nargs = lua_gettop(L);  // Get number of arguments

  // First argument (required): text
  if (nargs < 1) {
    return luaL_error(L, "print requires at least 1 argument (text)");
  }
  const char* text = luaL_checkstring(L, 1);
  int i = 2;
  int x = (float)luaL_checknumber(L, i++);
  int y = (float)luaL_checknumber(L, i++);
  int r = (float)luaL_checknumber(L, i++);
  int g = (float)luaL_checknumber(L, i++);
  int b = (float)luaL_checknumber(L, i++);
  int a = (float)luaL_checknumber(L, i++);
  font_draw_texture(text, x, y, (Color){r, g, b, a});
  return 0;
}

Buffer lua_tobuffer(lua_State* L, int idx) {
  LuaLevel* l = lua_getlevel(L);
  msgpack_sbuffer_clear(&l->sbuf);
  lua_to_msgpack(L, idx, &l->pk);
  return (Buffer){
      .data = (u8*)l->sbuf.data,
      .size = l->sbuf.size,
      .cap = 0,
  };
}

static void msgpack_to_lua(lua_State* L, msgpack_object* obj) {
  switch (obj->type) {
    case MSGPACK_OBJECT_NIL:
      lua_pushnil(L);
      break;

    case MSGPACK_OBJECT_BOOLEAN:
      lua_pushboolean(L, obj->via.boolean);
      break;

    case MSGPACK_OBJECT_POSITIVE_INTEGER:
      lua_pushinteger(L, obj->via.u64);
      break;

    case MSGPACK_OBJECT_NEGATIVE_INTEGER:
      lua_pushinteger(L, obj->via.i64);
      break;

    case MSGPACK_OBJECT_FLOAT32:
    case MSGPACK_OBJECT_FLOAT64:
      lua_pushnumber(L, obj->via.f64);
      break;

    case MSGPACK_OBJECT_STR:
      lua_pushlstring(L, obj->via.str.ptr, obj->via.str.size);
      break;

    case MSGPACK_OBJECT_ARRAY:
      lua_createtable(L, obj->via.array.size, 0);
      for (uint32_t i = 0; i < obj->via.array.size; i++) {
        msgpack_to_lua(L, &obj->via.array.ptr[i]);
        lua_rawseti(L, -2, i + 1);
      }
      break;

    case MSGPACK_OBJECT_MAP:
      lua_createtable(L, 0, obj->via.map.size);
      for (uint32_t i = 0; i < obj->via.map.size; i++) {
        msgpack_to_lua(L, &obj->via.map.ptr[i].key);
        msgpack_to_lua(L, &obj->via.map.ptr[i].val);
        lua_settable(L, -3);
      }
      break;

    default:
      lua_pushnil(L);
      break;
  }
}

int lua_frombuffer(lua_State* L, Buffer buf) {
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);
  msgpack_unpack_return ret =
      msgpack_unpack_next(&msg, (char*)buf.data, buf.size, NULL);
  if (ret == MSGPACK_UNPACK_SUCCESS) {
    msgpack_to_lua(L, &msg.data);
    msgpack_unpacked_destroy(&msg);
    return 1;  // Success
  }
  msgpack_unpacked_destroy(&msg);
  return 0;  // Failure
}

#define WRAP(f) lua_register(L, #f, lua_##f);

static Status init_level_lua(LuaLevel* lvl) {
  lua_State* L = luaL_newstate();
  if (!L) {
    return status_error("Failed to create Lua state\n");
  }
  lvl->L = L;
  luaL_openlibs(L);
  lua_register(L, "caPrint", l_print);
  lua_register(L, "draw_font", lua_draw_font);
  lua_register(L, "draw_box", lua_draw_box);
  WRAP(rl_scalef);
  WRAP(rl_translatef);
  WRAP(rl_push_matrix);
  WRAP(rl_pop_matrix);
  WRAP(measure_text_size);
  lua_register(L, "pget", lua_pget);
  lua_register(L, "pset", lua_pset);
  lua_register(L, "draw_texture_pro", lua_draw_texture_pro);
  lua_register(L, "draw_rectangle_pro", lua_draw_rectangle_pro);
  lua_register(L, "notify_level_complete", lua_notify_level_complete);
  /* Stores level in lua for easy of access */
  lua_pushlightuserdata(L, lvl);
  lua_setfield(L, LUA_REGISTRYINDEX, "level_ptr");
  return status_ok();
}

static void lua_level_destroy(void* u) {
  LuaLevel* lvl = u;
  if (lvl->L) {
    lua_close(lvl->L);
    msgpack_sbuffer_destroy(&lvl->sbuf);
  }

  int na = arrlen(lvl->assets);
  for (int i = 0; i < na; i++) {
    if (lvl->assets[i].tex.width > 0) {
      UnloadTexture(lvl->assets[i].tex);
    }
  }
  arrfree(lvl->assets);
  lvl->L = NULL;
  free(lvl);
}

static Status lua_level_start(void* u, Sim* sim) {
  LuaLevel* lvl = u;
  lvl->sim = sim;
  lua_State* L = lvl->L;
  lua_getglobal(L, "_start");
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    return status_lua_error(L);
  }
  return status_ok();
}

static Status lua_level_draw(void* u) {
  LuaLevel* lvl = u;
  lua_State* L = lvl->L;
  lua_getglobal(L, "_draw");
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    return status_lua_error(L);
  }
  return status_ok();
}

#if 0
Status level_draw_board(Level* lvl) {
  lua_State* L = lvl->L;
  assert(L);
  lua_getglobal(L, "_drawboard");
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    return status_lua_error(L);
  }
  return status_ok();
}
#endif

Status lua_level_update(void* u, Buffer* buffer) {
  LuaLevel* lvl = u;
  lua_State* L = lvl->L;
  lua_getglobal(L, "_update");
  if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
    return status_lua_error(L);
  }
  *buffer = lua_tobuffer(L, -1);
  lua_pop(L, 1);
  return status_ok();
}

static Status lua_level_fw(void* u, Buffer buf) {
  LuaLevel* lvl = u;
  lua_State* L = lvl->L;
  lua_getglobal(L, "_forward");
  int r = lua_frombuffer(L, buf);
  if (r == 0) {
    return status_ok();
  }
  assert(r);
  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    return status_lua_error(L);
  }
  return status_ok();
}

static Status lua_level_bw(void* u, Buffer buf) {
  LuaLevel* lvl = u;
  lua_State* L = lvl->L;
  lua_getglobal(L, "_backward");
  int r = lua_frombuffer(L, buf);
  if (r == 0) {
    return status_ok();
  }
  assert(r);
  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    return status_lua_error(L);
  }
  return status_ok();
}

Status lua_level_create(LevelAPI* api, LevelDef* ldef) {
  LuaLevel* lvl = calloc(1, sizeof(LuaLevel));
  *api = (LevelAPI){0};
  api->u = lvl;
  api->start = lua_level_start;
  api->update = lua_level_update;
  api->fw = lua_level_fw;
  api->bw = lua_level_bw;
  api->draw = lua_level_draw;
  api->destroy = lua_level_destroy;

  lvl->ldef = ldef;
  Status status = init_level_lua(lvl);
  if (!status.ok) return status;
  msgpack_sbuffer_init(&lvl->sbuf);
  msgpack_packer_init(&lvl->pk, &lvl->sbuf, msgpack_sbuffer_write);
  if (ldef) {
    int nk = arrlen(ldef->kernels);
    for (int i = 0; i < nk; i++) {
      printf("Loading %s ...\n", ldef->kernels[i]);
      if (luaL_dofile(lvl->L, ldef->kernels[i]) != LUA_OK) {
        return status_lua_error(lvl->L);
      }
    }
    /* Loading assets*/
    int na = arrlen(ldef->assets);
    for (int i = 0; i < na; i++) {
      printf("Loading Asset %s ...\n", ldef->assets[i]);
      // TODO: check if ends with ".png", and if the file exists..
      LevelAsset asset = {0};
      asset.tex = LoadTexture(ldef->assets[i]);
      arrput(lvl->assets, asset);
    }
  }

  lua_getglobal(lvl->L, "_setup");
  if (lua_pcall(lvl->L, 0, 0, 0) != LUA_OK) {
    return status_lua_error(lvl->L);
  }
  if (lvl->ldef) {
    lua_level_init_pins(lvl, api);
  }
  return status_ok();
}

