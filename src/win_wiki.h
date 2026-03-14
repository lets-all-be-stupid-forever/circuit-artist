#ifndef CA_TUTORIAL_H
#define CA_TUTORIAL_H
#include "common.h"
#include "game_registry.h"

void win_wiki_init(GameRegistry* r);
void win_wiki_open();
void win_wiki_open_on_item(const char* item_name);
void win_wiki_update();
void win_wiki_draw();

#endif
