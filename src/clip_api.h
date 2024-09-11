#ifndef CLIP_API_H
#define CLIP_API_H
#include "raylib.h"

#if defined(__cplusplus)
extern "C" {
#endif
#include "img.h"
#include "ui.h"
#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C" {  // Prevents name mangling of functions
#endif

Image ImageFromClipboard();
void ImageToClipboard(Image img);

#if defined(__cplusplus)
}
#endif

#endif
