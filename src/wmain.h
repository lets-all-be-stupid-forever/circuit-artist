#ifndef CA_WMAIN_H
#define CA_WMAIN_H
#include "blueprint.h"
#include "game_registry.h"
#include "paint.h"
#include "pin_spec.h"
#include "raylib.h"

#define MODE_EDIT 0
#define MODE_SIMU 1
#define MODE_ERROR 2

typedef void (*Callback)();

// Main screen.
void main_init(GameRegistry* registry);
void main_open();
void main_update();
void main_draw();
void main_destroy();
void main_set_palette_from_image(Image img);
void main_ask_for_save_and_proceed(Callback next_action);
void main_paste_text(const char* txt);
void main_paste_file(const char* fname, int rot);
void main_set_line_sep(int n);
int main_on_save_click(bool saveas);
void main_load_image_from_path(const char* path);
void main_load_blueprint(Blueprint* bp);
void main_start_simu();
Blueprint* main_get_editting_blueprint();
void main_stop_simu();
Paint* main_get_paint();
bool is_circuit_sound_on();
bool is_paint_sound_on();
void main_select_custom_level(CustomLevelDef* c);
bool custom_level_open_file();
void main_load_campaign_level(LevelDef* ldef);
void main_load_custom_level(CustomLevelDef* ldef);
void main_load_file_level(const char* fname);

typedef struct {
  bool done;
  bool error;
} SimuState;

SimuState main_get_simu_state();

#endif
