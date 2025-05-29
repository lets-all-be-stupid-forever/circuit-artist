#ifndef W_MAIN_H
#define W_MAIN_H
#include "paint.h"
#include "ui.h"

// Main screen.
void MainOpen(Ui* ui);
void MainUpdate(Ui* ui);
void MainDraw(Ui* ui);

typedef void (*UiCallback)(Ui*);

Paint* MainGetPaint();
void MainUnload();
void MainSetPaletteFromImage(Image img);
void MainAskForSaveAndProceed(Ui* ui, UiCallback next_action);
void MainPasteText(const char* txt);
void MainSetLineSep(int n);
int MainOnSaveClick(Ui* ui, bool saveas);

#endif
