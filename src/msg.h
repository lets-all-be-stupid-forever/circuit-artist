#ifndef MSG_H
#define MSG_H
#include "ui.h"

// -----------------------------------------
// Little messaging system for the main page.
// -----------------------------------------

// Adds a message that will expire within `duration` seconds.
void MsgAdd(const char* msg_txt, float duration);

// Draws messages on screen.
void MsgDraw(Ui* ui);

// Updates lifetime of messages.
void MsgUpdate();

#endif
