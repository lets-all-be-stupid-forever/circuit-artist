#ifndef CA_WABOUT_H
#define CA_WABOUT_H
#include "common.h"

// About screen.
void easy_about_open();
void easy_blinking_open();
void about_init();
void about_open(const char* title, const char* content, sprite_t* sprites);
void about_update();
void about_draw();

#endif
