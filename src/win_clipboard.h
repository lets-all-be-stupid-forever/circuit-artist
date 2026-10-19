#ifndef WIN_CLIPBOARD_H
#define WIN_CLIPBOARD_H
#include "common.h"

void win_clipboard_init();
void win_clipboard_open();
void win_clipboard_update();
void win_clipboard_draw();

bool win_clipboard_can_paste();
void win_clipboard_queue_paste(const unsigned char* data, int len);

#endif
