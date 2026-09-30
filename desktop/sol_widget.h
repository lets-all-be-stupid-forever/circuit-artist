#ifndef CA_SOL_WIDGET_H
#define CA_SOL_WIDGET_H
#include "blueprint.h"
#include "common.h"
#include "game_registry.h"
#include "layout.h"

/* Having a file for this solution widget since it's used in 2 pages */
typedef struct {
  Btn sol[4];
  Btn btn_prv;
  Btn btn_nxt;
  Btn btn_browse;
  char* level_id;
  Blueprint** bps;
  int page;
} SolWidget;

void sol_widget_init(SolWidget* s);
void sol_widget_set_level_id(SolWidget* s, const char* level_id);
void sol_widget_update_layout(SolWidget* s, Layout* l);
void sol_widget_draw(SolWidget* s);
void sol_widget_draw_leg(SolWidget* s);
void sol_widget_update(SolWidget* s);

#endif
