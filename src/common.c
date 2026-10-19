#include "common.h"

#include "paths.h"
#include "stdlib.h"
#include "utils.h"

static struct {
  Image img_sprites;  // Global UI sprites loaded from the sprite4.png asset
  Texture2D sprites;  // Global UI sprites loaded from the sprite4.png asset
  int scale;          // Global UI pixel scaling.
  int argc;
  char** argv;
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

void register_args(int argc, char** argv) {
  if (argc == 0) return;
  C.argc = argc;
  C.argv = calloc(argc, sizeof(char*));
  for (int i = 0; i < argc; i++) {
    C.argv[i] = clone_string(argv[i]);
  }
}

const char* get_cli_arg(const char* str) {
  for (int i = 0; i < C.argc; i++) {
    if (strcmp(str, C.argv[i]) == 0) {
      if (i == C.argc - 1) {
        return NULL;
      }
      return C.argv[i + 1];
    }
  }
  return NULL;
}

bool ui_is_demo() {
#if defined(PLATFORM_WEB)
  return true;
#else
  return false;
#endif
}
