#ifndef CA_WIN_MSG_H
#define CA_WIN_MSG_H
#include "common.h"

void win_msg_open_tex(Texture tex, int scale);
void win_msg_open_text(const char* txt, sprite_t* sprites);
void win_msg_update();
void win_msg_draw();

#endif
