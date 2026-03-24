#ifndef CA_WIN_BLUEPRINT_H
#define CA_WIN_BLUEPRINT_H
#include "common.h"

/* Blueprints */
typedef struct {
  char* id;
  char* name;
  char* folder;
  int rot;
  Texture thumbnail;
} Blueprint;

/* returns index of the new blueprint, otherwise -1 if failed */
int blueprint_create_and_open(int nl, Image* imgs, Image full);

void win_blueprint_init();
void win_blueprint_open();
void win_blueprint_update();
void win_blueprint_draw();

#endif
