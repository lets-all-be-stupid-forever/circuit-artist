#ifndef CA_ASSET_PATH_H
#define CA_ASSET_PATH_H

#include "raylib.h"

void asset_path_init();
char* get_asset_path(const char* path);
Sound load_sound_asset(const char* asset);
Image load_image_asset(const char* asset);
Texture load_texture_asset(const char* asset);
Shader load_shader_asset(const char* vs_asset, const char* fs_asset);

#endif
