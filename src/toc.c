#include "toc.h"

#include <ctype.h>
#include <string.h>

static const char* skip_eol(const char* p) {
  if (*p == '\r' && p[1] == '\n') return p + 2;
  if (*p == '\r' || *p == '\n') return p + 1;
  return p;
}

static size_t copy_trimmed(char* dst, size_t dst_size, const char* start,
                           const char* end) {
  // Trim trailing whitespace
  while (end > start && isspace((unsigned char)*(end - 1))) end--;

  size_t len = (size_t)(end - start);

  if (dst_size == 0) return 0;

  size_t copy_len = (len < dst_size - 1) ? len : dst_size - 1;
  memcpy(dst, start, copy_len);
  dst[copy_len] = '\0';

  return copy_len;
}

const char* next_header(const char* s, char* key, size_t key_size, char* value,
                        size_t value_size) {
  const char* p = s;

  if (key_size) key[0] = '\0';
  if (value_size) value[0] = '\0';

  if (!p || *p == '\0') return s;

  // Must start with ##
  if (p[0] != '#' || p[1] != '#') return s;

  p += 2;

  // Skip spaces (not EOL)
  while (*p && isspace((unsigned char)*p) && *p != '\n' && *p != '\r') p++;

  // Parse key
  const char* key_start = p;
  while (*p && *p != ':' && *p != '\n' && *p != '\r') p++;

  if (*p != ':') return s;

  const char* key_end = p;

  copy_trimmed(key, key_size, key_start, key_end);

  p++;  // skip ':'

  // Skip spaces before value
  while (*p && isspace((unsigned char)*p) && *p != '\n' && *p != '\r') p++;

  const char* val_start = p;

  // Read until EOL
  while (*p && *p != '\n' && *p != '\r') p++;

  const char* val_end = p;

  copy_trimmed(value, value_size, val_start, val_end);

  // Move to next line
  p = skip_eol(p);

  return p;
}

void toc_init(Toc* toc) {
  toc->entries = NULL;
  toc->count = 0;
  toc->capacity = 0;
}

static int toc_add(Toc* toc, const char* key, const char* value) {
  if (toc->count == toc->capacity) {
    size_t new_cap = (toc->capacity == 0) ? 8 : toc->capacity * 2;

    TocEntry* new_entries = realloc(toc->entries, new_cap * sizeof(TocEntry));
    if (!new_entries) return 0;

    toc->entries = new_entries;
    toc->capacity = new_cap;
  }

  toc->entries[toc->count].key = strdup(key);
  toc->entries[toc->count].value = strdup(value);

  if (!toc->entries[toc->count].key || !toc->entries[toc->count].value)
    return 0;

  toc->count++;
  return 1;
}

void toc_free(Toc* toc) {
  for (size_t i = 0; i < toc->count; i++) {
    free(toc->entries[i].key);
    free(toc->entries[i].value);
  }
  free(toc->entries);

  toc->entries = NULL;
  toc->count = 0;
  toc->capacity = 0;
}

int toc_parse(const char* text, Toc* toc, const char** out_body) {
  char key[256];
  char value[256];

  const char* p = text;

  while (1) {
    const char* next = next_header(p, key, sizeof(key), value, sizeof(value));

    if (next == p) break;  // no more headers

    if (!toc_add(toc, key, value)) return 0;  // allocation failure

    p = next;
  }

  if (out_body) *out_body = p;

  return 1;
}

const char* toc_get(const Toc* toc, const char* key) {
  for (size_t i = 0; i < toc->count; i++) {
    if (strcmp(toc->entries[i].key, key) == 0) return toc->entries[i].value;
  }
  return NULL;
}
