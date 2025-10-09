#include "api.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>

#include "font.h"
#include "msg.h"
#include "ui.h"
#include "utils.h"
#include "w_main.h"

// Global Lua context
static lua_State* _L = NULL;

// Parameters of the call to update of a level component.
typedef struct {
  // component number
  int ic;
  // previous input wires bits
  int* prev_in;
  // current input wires bits
  int* next_in;
  // output wires bits
  int* output;
  // whether the power_on_reset is on (in this case we might want to handle the
  // update differently)
  bool reset;
} ComponentUpdateCtx;

// Struct that is accessible from lua as a singleton to handle communication
// between lua and C.
typedef struct {
  // total clock time elapsed, modified from lua, but reseted from C.
  float elapsed;
  // size of a clock cycle in seconds. (set from C, read from lua's clock)
  float clock_time;
  // used in  "ApiOnLevelTick" to describe how much time has passed since last
  // tick call.
  float dt;
  // Camera X position.
  float cx;
  // Camera Y position.
  float cy;
  // Camera spacing.
  float cs;
  // Target rendering texture for components in lua.
  RenderTexture2D rt;
  // Parameter of  ApiLoadLevel.
  int requested_level;
  // Used on ApiOnLevelTick.
  int clock_update_value;
  // Used on ApiOnLevelTick.
  int clock_count;
  // whether power_on_reset flag is on. (set from lua)
  bool por;
  // Descriptor of the current active level.
  // Updated from the lua side.
  LevelDesc level_desc;
  // Global store of levels and tutorial page.
  // Set from lua.
  LevelOptions level_options;
  // Data used during call of component update. (ApiUpdateLevelComponent)
  ComponentUpdateCtx update_ctx;
} SharedState;

// Singleton used in the communication between lua and C.
static SharedState shared_state = {0};

#define LuaCall(func) RunOrCrash(luaL_dostring(_L, func))

CA_API SharedState* GetSharedState();

// workaround for MAC's luajit bug:
// NYI: cannot call this C function (yet)
// https://github.com/LuaJIT/LuaJIT/issues/205
typedef struct {
  Texture2D texture;
  Rectangle source;
  Rectangle dest;
  Vector2 origin;
  float rotation;
  Color tint;
} RlDrawTextureProArgs;

CA_API void RlDrawTexturePro(RlDrawTextureProArgs* args) {
  DrawTexturePro(args->texture, args->source, args->dest, args->origin,
                 args->rotation, args->tint);
}

CA_API void CaDrawText(const char* txt, int x, int y, Color c) {
  FontDrawTexture(txt, x, y, c);
}

CA_API void CaAddMessage(const char* txt, float duration) {
  MsgAdd(txt, duration);
}

CA_API void CaSetPalFromImage(Image img) { MainSetPaletteFromImage(img); }

CA_API void CaSetStartupImage(const char* path) {
  if (shared_state.level_options.startup_image_path) {
    free(shared_state.level_options.startup_image_path);
  }
  shared_state.level_options.startup_image_path = CloneString(path);
}

CA_API int CaGetDrawTextSize(const char* txt) {
  return GetRenderedTextSize(txt).x;
}

CA_API void CaDrawTextBox(const char* txt, int x, int y, int w, Color c) {
  DrawTextBox(txt, (Rectangle){x, y, w, 0}, c, NULL);
}

static void RunOrCrash(int status) {
  if (status != LUA_OK) {
    const char* error_msg = lua_tostring(_L, -1);
    fprintf(stderr, "Error in Lua:\n%s\n", error_msg);
    TraceLog(LOG_ERROR, "Lua Error: %s", error_msg);
    // Write to a crash log file
    FILE* f = fopen("crash.log", "w");
    if (f) {
      fprintf(f, "Lua Error:\n%s\n", error_msg);
      fclose(f);
    }
    exit(1);
  }
}

SharedState* GetSharedState() { return &shared_state; }

void ApiLoad() {
  _L = luaL_newstate();  // open Lua
  lua_State* L = _L;
  if (!L) {
    printf("Couldn't load LUA.");
    return;
  }
  luaL_openlibs(L);  // load Lua libraries
  RunOrCrash(luaL_dofile(L, "../luasrc/app.lua"));
}

void ApiUnload() {
  LuaCall("apiOnExit()");
  if (_L) {
    lua_close(_L);
  }
  _L = NULL;
}

void ApiLoadLevel(int i) {
  shared_state.requested_level = i;
  LuaCall("apiLoadLevel()");
}

void ApiStartLevelSimulation() { LuaCall("apiStartSimulation()"); }

void ApiStopLevelSimulation() { LuaCall("apiStopSimulation()"); }

void ApiOnLevelDraw(RenderTexture2D target, float camera_x, float camera_y,
                    float camera_spacing) {
  shared_state.rt = target;
  shared_state.cx = camera_x;
  shared_state.cy = camera_y;
  shared_state.cs = camera_spacing;
  LuaCall("apiOnLevelDraw()");
}

LevelDesc* ApiGetLevelDesc() { return &shared_state.level_desc; }

void ApiUpdateLevelComponent(void* ctx, int ic, int* prev_in, int* next_in,
                             int* output) {
  shared_state.update_ctx.ic = ic;
  shared_state.update_ctx.prev_in = prev_in;
  shared_state.update_ctx.next_in = next_in;
  shared_state.update_ctx.output = output;
  LuaCall("apiUpdateLevelComponent()");
}

LevelOptions* ApiGetLevelOptions() { return &shared_state.level_options; }

TickResult ApiOnLevelTick(float dt) {
  shared_state.dt = dt;
  LuaCall("apiOnLevelTick()");
  TickResult tr;
  tr.reset = shared_state.por;
  tr.clock_value = shared_state.clock_update_value;
  if (tr.clock_value >= 0) {
    tr.clock_updated = true;
  } else {
    tr.clock_updated = false;
  }
  tr.clock_count = shared_state.clock_count;
  return tr;
}

void ApiLevelClock(int ilevel, int* inputs, bool reset) {
  shared_state.update_ctx.ic = ilevel;
  shared_state.update_ctx.next_in = inputs;
  shared_state.update_ctx.reset = reset;
  LuaCall("apiOnLevelClock()");
}

void ApiSetClockTime(float clock_time) {
  shared_state.elapsed = 0;
  shared_state.clock_time = clock_time;
}
