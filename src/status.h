#ifndef CA_STATUS_H
#define CA_STATUS_H
#include <lua.h>

#include "utils.h"

typedef struct {
  bool ok;
  char* err_msg;
} Status;

static inline Status status_ok() {
  return (Status){.ok = true, .err_msg = NULL};
}

static inline Status status_error(const char* msg) {
  return (Status){.ok = false, .err_msg = clone_string(msg)};
}

#endif
