#ifndef W_DIALOG_H
#define W_DIALOG_H
#include "defs.h"
#include "ui.h"
#include "w_main.h"

// Dialog screen.
void DialogOpen(Ui* ui, const char* modal_msg, UiCallback modal_next_action);
void DialogUpdate(Ui* ui);
void DialogDraw(Ui* ui);

#endif
