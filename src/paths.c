#include "paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRESS_FILE "save_1_1.json"
#define STAMPS_FILE "blueprints_1_1.json"

static struct {
  const char* asset_path;
  const char* data_path;
} C = {
    .asset_path = "../assets",
    .data_path = "..",
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

  init_env_path(&C.data_path, "CA_DATA_DIR");
  if (!DirectoryExists(C.data_path) &&
      // MakeDirectory returns a 0 even if it failed to create the directory,
      // so after running it check again
      (MakeDirectory(C.data_path) != 0 || !DirectoryExists(C.data_path))) {
    fprintf(stderr,
            "\e[31m"
            "ERROR"
            "\e[m"
            ": Failed to create directory $CA_DATA_DIR. (%s)\n",
            C.data_path);
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

/// WARNING: This uses raylib's TextFormat function. Strings returned will
/// expire after this or TextFormat is called MAX_TEXTFORMAT_BUFFERS times
const char* get_data_path(const char* path) {
  return TextFormat("%s/%s", C.data_path, path);
}

const char* get_progress_path() { return get_data_path(PROGRESS_FILE); }
const char* get_stamp_path() { return get_data_path(STAMPS_FILE); }
