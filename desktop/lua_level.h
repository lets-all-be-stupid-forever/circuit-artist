#ifndef CA_LEVEL_H
#define CA_LEVEL_H

#include "buffer.h"
#include "common.h"
#include "game_registry.h"
#include "pin_spec.h"
#include "sim.h"
#include "status.h"
#include "stdbool.h"

Status lua_level_create(LevelAPI* lvl, LevelDef* ldef);

#endif
