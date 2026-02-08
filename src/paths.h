#ifndef CA_PATHS_H
#define CA_PATHS_H

#include "raylib.h"

void paths_init();

char* get_asset_path(const char* path);
Sound load_sound_asset(const char* asset);
Image load_image_asset(const char* asset);
Texture load_texture_asset(const char* asset);
Shader load_shader_asset(const char* vs_asset, const char* fs_asset);

const char* get_data_path(const char* path);
const char* get_progress_path();
const char* get_stamp_path();

#endif
