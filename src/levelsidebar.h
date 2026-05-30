#ifndef CA_LEVEL_SIDEBAR_H
#define CA_LEVEL_SIDEBAR_H
#include "common.h"
#include "game_registry.h"
#include "layout.h"
#include "widgets.h"

typedef struct {
  Layout* layout;
  Rectangle level_title;
  LevelDef* ldef;
  Textbox tb;
  Btn btn_msg[8];
  Btn btn_close;
  Rectangle region;
  void (*on_sidebar_close)();
} LevelSidebarWidget;

void level_sidebar_init(LevelSidebarWidget* w);
void level_sidebar_set_level(LevelSidebarWidget* w, LevelDef* ldef);
void level_sidebar_update(LevelSidebarWidget* w);
void level_sidebar_update_layout(LevelSidebarWidget* w, Rectangle region);
void level_sidebar_draw(LevelSidebarWidget* w);
void level_sidebar_draw_leg(LevelSidebarWidget* w);

#endif
