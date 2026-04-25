#ifndef CA_WIN_PUBFORM_H
#define CA_WIN_PUBFORM_H

typedef struct {
  const char* folder;
  const char* default_title;
  const char* default_desc;
  const char* default_thumbnail_path;
} PubformParams;

void win_pubform_init();
void win_pubform_open(PubformParams params);
void win_pubform_update();
void win_pubform_draw();

#endif
