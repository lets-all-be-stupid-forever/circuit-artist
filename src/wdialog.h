#ifndef CA_WDIALOG_H
#define CA_WDIALOG_H

// Dialog screen.
void dialog_open(const char* modal_msg, const char* opt0, const char* opt1,
                 const char* opt2, void (*modalNxtAction)(int r));
void dialog_update();
void dialog_draw();

#endif

