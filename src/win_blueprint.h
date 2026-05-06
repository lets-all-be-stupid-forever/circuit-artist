#ifndef CA_WIN_BLUEPRINT_H
#define CA_WIN_BLUEPRINT_H
#include "blueprint.h"
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* returns index of the new blueprint, otherwise -1 if failed */
int blueprint_create_and_open(int nl, Image* imgs, Image full, const char* lvl);
void add_steam_blueprint(const char* folder, u64 id);

void win_blueprint_init();
void win_blueprint_open();
void win_blueprint_update();
void win_blueprint_draw();
void get_linked_blueprints(Blueprint*** sols, const char* id);

#if defined(__cplusplus)
}
#endif

#endif
