#ifndef SPRITE_H
#define SPRITE_H
#include "raylib.h"

// Simple descriptor of a sprite.
typedef struct {
  Texture2D tex;
  Rectangle region;
} Sprite;

#endif
