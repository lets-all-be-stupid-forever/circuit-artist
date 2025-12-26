#ifndef CA_WTEXT_H
#define CA_WTEXT_H

// Text input screen.
void text_modal_open(void (*on_accept)(void* ctx, const char* txt), void* ctx,
                     const char* initial_txt);
void text_modal_update();
void text_modal_draw();

#endif
