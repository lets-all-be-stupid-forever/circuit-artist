#ifndef CA_FS_H
#define CA_FS_H

#include "stdbool.h"

#if defined(__cplusplus)
extern "C" {
#endif

char* checkmodpath(const char* root, const char* path);
char* checkmodpath2(const char* root, const char* caller, const char* path);
char* abs_path(const char* path);  // malloc'd, caller must free()
char* os_path_basename(const char* fname);
bool os_path_exists(const char* path);
void ensure_folder_exists(const char* path);
char* os_path_join_impl(const char* first, ...);

#if defined(__cplusplus)
}
#endif

#define os_path_join(...) os_path_join_impl(__VA_ARGS__, (const char*)NULL)

#endif
