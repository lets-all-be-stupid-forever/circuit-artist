#ifndef CA_VIEWER_H
#define CA_VIEWER_H
#include "raylib.h"

int doit(Image img);
void player_init();
void player_update(double frame_time);
void player_render(RenderTexture2D* target);

#endif
