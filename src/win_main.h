#ifndef CA_WMAIN_H
#define CA_WMAIN_H
#include "blueprint.h"
#include "game_registry.h"

#define MODE_EDIT 0
#define MODE_SIMU 1
#define MODE_ERROR 2

// Main screen.
void win_main_init();
void win_main_open();
void win_main_update();
void win_main_draw();
void win_main_destroy();
void win_main_ask_for_save_and_proceed(void (*next_action)());
void win_main_paste_file(const char* fname, int rot);
void win_main_set_line_sep(int n);
void win_main_load_image_from_path(const char* path);
void win_main_load_blueprint(Blueprint* bp);
bool win_main_custom_level_open_file();
void win_main_start_simu();
void win_main_stop_simu();
void win_main_load_custom_level(CustomLevelDef* ldef);
void win_main_open_level();
bool win_main_is_simu_error();
bool win_main_is_simu_done();
Blueprint* win_main_get_editting_blueprint();

#endif
