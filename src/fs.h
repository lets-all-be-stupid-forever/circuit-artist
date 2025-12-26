#ifndef CA_FS_H
#define CA_FS_H

#if defined(__cplusplus)
extern "C" {
#endif

char* checkmodpath(const char* root, const char* path);
char* checkmodpath2(const char* root, const char* caller, const char* path);

#if defined(__cplusplus)
}
#endif

#endif
