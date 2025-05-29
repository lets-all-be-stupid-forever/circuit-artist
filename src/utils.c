#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// Implementation of the stb_ds library.
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

char* CloneString(const char* str) {
  size_t len = strlen(str) + 1;
  char* p = (char*)malloc(len);
  memmove(p, str, len);
  return p;
}

bool IsControlDown() {
  return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_LEFT_SUPER);
}

