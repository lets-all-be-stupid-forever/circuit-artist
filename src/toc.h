#ifndef CA_TOC_H
#define CA_TOC_H

#include <stdlib.h>
#include <string.h>

typedef struct {
  char* key;
  char* value;
} TocEntry;

/* Very simple toc parser */
typedef struct {
  TocEntry* entries;
  size_t count;
  size_t capacity;
} Toc;

void toc_init(Toc* toc);
void toc_free(Toc* toc);
int toc_parse(const char* text, Toc* toc, const char** out_body);
const char* toc_get(const Toc* toc, const char* key);

#endif
