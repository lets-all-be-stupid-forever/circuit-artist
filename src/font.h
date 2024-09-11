#ifndef FONT_H
#define FONT_H
#include "img.h"
#include "sprite.h"

// Loads font from a dedicated file.
// It's not stable, the font format needs to move towards what raylib is using.
void LoadArtFont(const char* filepath);

// Destructor.
void UnloadArtFont();

// Simple text drawing in an image.
void FontDraw(Image* dst, const char* txt, int x, int y, Color c);

// Draws a text line directly in the main screen.
void FontDrawTexture(const char* txt, int x, int y, Color c);

// Draws an outlined text line in main screen.
void FontDrawTextureOutlined(const char* txt, int x, int y, Color c, Color bg);

// Draws a text box.
// If a height pointer is passed, instead of drawing it computes the height of
// the rendered text.
void DrawTextBox(const char* text, Rectangle rect, Color c, int* height);

// Computes the height of a rendered textbox.
void GetDrawTextBoxSize(const char* text, int lw, int* h, int* w);

// Returns the font line height in pixels.
int GetFontLineHeight();

// Computes the x and y size of a rendered text line.
Vector2 GetRenderedTextSize(const char* txt);

// Renders a text line into an image. The caller should take ownership of the
// image.
Image RenderText(const char* txt, Color c);

// Advanced text box rendering.
// It can render:
// - Highlighted text via `blabla`
// - Images via !img:<number> --> uses the number-th sprite passed as input in `sprites`.
// It is used for both circuitopedia and level description rendering.
void DrawTextBoxAdvanced(const char* text, Rectangle rect, Color c, Sprite* sprites, int* height);

#endif
