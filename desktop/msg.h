#ifndef CA_MSG_H
#define CA_MSG_H

void msg_init();
void msg_add(const char* msg_txt, float msg_duration);
void msg_clear_permanent();
void msg_draw();
void msg_update();

#endif
