#ifndef CA_WIN_MTEXT_H
#define CA_WIN_MTEXT_H

void win_mtext_init();
void win_mtext_open(void (*on_accept)(void* ctx, const char* txt), void* ctx,
                    const char* initial_txt);
void win_mtext_update();
void win_mtext_draw();

#endif
