#include "clip_api.h"

#include <clip.h>

#include "img.h"

Image ImageFromClipboard() {
  clip::image img;
  if (!clip::get_image(img)) {
    // std::cout << "Error getting image from clipboard\n";
    Image r = {0, 0, 0, 0, 0};
    return r;
  }
  clip::image_spec spec = img.spec();
  int w = spec.width;
  int h = spec.height;
  Image ret = GenImageSimple(w, h);
  Color* colors = GetPixels(ret);
  int byte_stride = spec.bits_per_pixel >> 3;
  if (spec.alpha_mask != 0) {
    for (unsigned long y = 0; y < spec.height; ++y) {
      char* src = (img.data() + y * spec.bytes_per_row);
      for (unsigned long x = 0; x < spec.width; ++x) {
        const uint32_t c = *(uint32_t*)(src + byte_stride * x);
        int r = ((c & spec.red_mask) >> spec.red_shift);
        int g = ((c & spec.green_mask) >> spec.green_shift);
        int b = ((c & spec.blue_mask) >> spec.blue_shift);
        int a = ((c & spec.alpha_mask) >> spec.alpha_shift);
        colors[y * w + x].r = r;
        colors[y * w + x].g = g;
        colors[y * w + x].b = b;
        colors[y * w + x].a = a;
      }
    }
  } else {
    for (unsigned long y = 0; y < spec.height; ++y) {
      char* src = (img.data() + y * spec.bytes_per_row);
      for (unsigned long x = 0; x < spec.width; ++x) {
        const uint32_t c = *(uint32_t*)(src + byte_stride * x);
        uint32_t r = ((c & spec.red_mask) >> spec.red_shift);
        uint32_t g = ((c & spec.green_mask) >> spec.green_shift);
        uint32_t b = ((c & spec.blue_mask) >> spec.blue_shift);
        colors[y * w + x].r = r;
        colors[y * w + x].g = g;
        colors[y * w + x].b = b;
        colors[y * w + x].a = 255;
      }
    }
  }
  return ret;
}

void ImageToClipboard(Image img) {
  clip::image_spec spec;
  spec.width = img.width;
  spec.height = img.height;
  spec.bytes_per_row = 4 * img.width;
  spec.bits_per_pixel = 32;
  spec.red_mask = 0xff;
  spec.green_mask = 0xff00;
  spec.blue_mask = 0x00ff0000;
  spec.alpha_mask = 0xff000000;
  spec.red_shift = 0;
  spec.green_shift = 8;
  spec.blue_shift = 16;
  spec.alpha_shift = 24;
  clip::image clip_img((void*)GetPixels(img), spec);
  clip::set_image(clip_img);
}
