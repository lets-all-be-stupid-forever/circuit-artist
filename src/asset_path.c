#include "asset_path.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* asset_path = "..";

void asset_path_init() {
  const char* new_asset_path = getenv("CA_ASSET_PATH");
  if (new_asset_path == NULL) return;

  int len = strlen(new_asset_path);
  if (len == 0) return;

  // ensure path doesn't end with a path separator
  if (new_asset_path[len - 1] == '/' || new_asset_path[len - 1] == '\\')
    new_asset_path -= 1;

  asset_path = new_asset_path;
}

/// The caller must free the returned C-String
char* get_asset_path(const char* path) {
  // XXX: should this just return a pointer to a static buffer?
  char* buf = malloc(strlen(asset_path) + 1 + strlen(path) + 1);

  sprintf(buf, "%s/%s", asset_path, path);

  return buf;
}

Sound load_sound_asset(const char* asset) {
  char* path = get_asset_path(asset);

  Sound result = LoadSound(path);

  free(path);

  return result;
}

Image load_image_asset(const char* asset) {
  char* path = get_asset_path(asset);

  Image result = LoadImage(path);

  free(path);

  return result;
}

Texture load_texture_asset(const char* asset) {
  char* path = get_asset_path(asset);

  Texture result = LoadTexture(path);

  free(path);

  return result;
}

Shader load_shader_asset(const char* vs_asset, const char* fs_asset) {
  char *vs_path = NULL, *fs_path = NULL;

  if (vs_asset != NULL) vs_path = get_asset_path(vs_asset);
  if (fs_asset != NULL) fs_path = get_asset_path(fs_asset);

  Shader result = LoadShader(vs_path, fs_path);

  free(vs_path);
  free(fs_path);

  return result;
}
