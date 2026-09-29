#include "common.h"

#include "paths.h"

static struct {
  Image img_sprites;  // Global UI sprites loaded from the sprite4.png asset
  Texture2D sprites;  // Global UI sprites loaded from the sprite4.png asset
  int scale;          // Global UI pixel scaling.

} C = {0};

int ui_get_scale() { return C.scale; }
Texture2D ui_get_sprites() { return C.sprites; };
Image ui_get_sprites_img() { return C.img_sprites; };

void init_common_globals() {
  //  For now the ui scale only works at scale=2, there are some hard coded
  //  scale that needs to be fixed (low priority)
  C.scale = 2;
  C.img_sprites = load_image_asset("imgs/sprite4.png");
  C.sprites = LoadTextureFromImage(C.img_sprites);
}
