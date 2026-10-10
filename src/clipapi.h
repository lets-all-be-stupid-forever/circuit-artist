#ifndef CA_CLIPAPI_H
#define CA_CLIPAPI_H
#include "img.h"

#if defined(__cplusplus)
extern "C" {
#endif

Image image_from_clipboard();
void image_to_clipboard(Image img);

#if defined(__cplusplus)
}
#endif

#endif
