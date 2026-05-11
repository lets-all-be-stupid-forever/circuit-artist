#ifndef CA_UIFONT_H
#define CA_UIFONT_H
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void uifont_load();
void uifont_unload();
void uifont_draw_image(Image* dst, const char* txt, int x, int y, Color c);
void uifont_draw_texture(const char* txt, int x, int y, Color c);
void uifont_draw_texture_big(const char* txt, int x, int y, Color c);
void uifont_draw_texture_bold(const char* txt, int x, int y, Color color);
void uifont_draw_texture_outlined(const char* txt, int x, int y, Color c,
                                  Color bg);
void uifont_draw_text_box(const char* text, Rectangle rect, Color c,
                          int* height);
void uifont_draw_text_box_advanced(const char* text, Rectangle rect, Color c,
                                   sprite_t* sprites, int* height);
void uifont_get_text_box_size(const char* text, int lw, int* h, int* w);
int uifont_line_height();
v2 uifont_text_size(const char* txt);
v2 uifont_text_size_big(const char* txt);
Image uifont_render_text(const char* txt, Color c);
Image uifont_render_text_1x(const char* txt, Color c);
int uifont_line_height_big();

#if defined(__cplusplus)
}
#endif

#endif
