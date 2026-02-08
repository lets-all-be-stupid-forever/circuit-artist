#include "paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
  const char* asset_path;
} C = {
    .asset_path = "../assets",
};

static void init_env_path(const char** path, const char* name);

void paths_init() {
  init_env_path(&C.asset_path, "CA_ASSET_DIR");
  if (!DirectoryExists(C.asset_path)) {
    fprintf(stderr,
            "\e[31m"
            "ERROR"
            "\e[m"
            ": $CA_ASSET_DIR must be a directory that exists. (%s)\n",
            C.asset_path);
    exit(EXIT_FAILURE);
  }
}

static void init_env_path(const char** path, const char* name) {
  const char* new_path = getenv(name);
  if (new_path == NULL) return;

  int len = strlen(new_path);
  if (len == 0) return;

  *path = new_path;
}

/// The caller must free the returned C-String
char* get_asset_path(const char* path) {
  // XXX: should this just return a pointer to a static buffer?
  char* buf = malloc(strlen(C.asset_path) + 1 + strlen(path) + 1);

  sprintf(buf, "%s/%s", C.asset_path, path);

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
