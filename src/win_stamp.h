#ifndef CA_WIN_STAMP_H
#define CA_WIN_STAMP_H
#include "common.h"

/* Stamps */
typedef struct {
  char* id;
  char* name;
  int rot;
  Texture thumbnail;
} Stamp;

/* returns index of the new stamp, otherwise -1 if failed */
int stamp_create_and_open(int nl, Image* imgs, Image full);

// Still need a "new stamp" window anad a "stamp detail" window
void win_stamp_init();
void win_stamp_open();
void win_stamp_update();
void win_stamp_draw();

#endif
