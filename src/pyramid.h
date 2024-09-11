#ifndef PYRAMID_H
#define PYRAMID_H
#include "img.h"

// Creates a downsampled version of the image.
// Used in the generation of multi-resolution image pyramids.
Image PyramidGenImage(Image base);

// Notifies that the pixel (x2, y2) of the pyramid has been updated and adjusts
// the lower-level pyramid pixel accordingly.
// `b` is the original image (that had the pixel changed)
// `s` is the downsampled pyramid image.
void PyramidUpdatePixel(Image b, Image* s, int x2, int y2);

// Notifies that a rectangular region in the image has changed and updates the pyramid levels.
// `n` is the number of pyramid levels.
// `h` is an array with each pyramid level. h[0] is the first level (higher
// resolution and the one containing the changes).
void PyramidUpdateRect2(int n, Image* h, RectangleInt r);

#endif
