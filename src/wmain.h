#ifndef CA_WMAIN_H
#define CA_WMAIN_H
#include "game_registry.h"
#include "paint.h"
#include "pin_spec.h"
#include "raylib.h"

#define MODE_EDIT 0
#define MODE_SIMU 1
#define MODE_ERROR 2

typedef enum {
  SOUND_LEVEL_COMPLETE,
} SoundEnum;

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
void main_start_simu();
void main_stop_simu();
Paint* main_get_paint();
void on_click();
void on_paint_act();
void play_sound(SoundEnum sound);

typedef struct {
  bool done;
  bool error;
} SimuState;

SimuState main_get_simu_state();

#endif
