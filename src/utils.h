#ifndef UTILS_H
#define UTILS_H
#include "stdbool.h"

// Does the same as strdup().
char* CloneString(const char* str);

// Checks for left control or macos command key down
bool IsControlDown();

#endif
