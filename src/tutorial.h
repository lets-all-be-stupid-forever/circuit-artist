#ifndef CA_TUTORIAL_H
#define CA_TUTORIAL_H
#include "common.h"
#include "game_registry.h"

void tutorial_init(GameRegistry* r);
void tutorial_open();
void tutorial_open_on_item(const char* item_name);
void tutorial_update();
void tutorial_draw();

#endif
