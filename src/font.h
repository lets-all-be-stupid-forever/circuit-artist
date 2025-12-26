#ifndef CA_FONT_H
#define CA_FONT_H
#include "common.h"

void load_art_font(const char* filepath);
void unload_art_font();
void font_draw(Image* dst, const char* txt, int x, int y, Color c);
void font_draw_texture(const char* txt, int x, int y, Color c);
void font_draw_texture_outlined(const char* txt, int x, int y, Color c,
                                Color bg);
void draw_text_box(const char* text, Rectangle rect, Color c, int* height);
void get_draw_text_box_size(const char* text, int lw, int* h, int* w);
int get_font_line_height();
v2 get_rendered_text_size(const char* txt);
Image render_text(const char* txt, Color c);
void draw_text_box_advanced(const char* text, Rectangle rect, Color c,
                            sprite_t* sprites, int* height);

#endif
