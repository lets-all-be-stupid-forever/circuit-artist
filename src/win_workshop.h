#ifndef CA_win_workshop_H
#define CA_win_workshop_H

#if defined(__cplusplus)
extern "C" {
#endif

void win_workshop_init();
void win_workshop_open(const char* level_id);
void win_workshop_update();
void win_workshop_draw();
void win_workshop_on_close();

#if defined(__cplusplus)
}
#endif

#endif
