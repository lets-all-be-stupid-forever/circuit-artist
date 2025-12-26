#ifndef CA_IMG_H
#define CA_IMG_H
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

// typedef struct {
//   u32 id;
//   int width;
//   int height;
// } IntTexture;
//
// IntTexture inttex_create(int w, int h);
// void inttex_update(IntTexture* t, int* data);
// void inttex_free(IntTexture* t);

Image gen_image_simple(int w, int h);
Image gen_image_filled(int w, int h, Color v);
Image gen_image_lut_heat();
Image clone_image(Image img);
Image crop_image(Image img, RectangleInt region);
Image rotate_image(Image img, int ccw);
void fill_image(Image* img, Color v);
void fill_image_rect(Image* img, RectangleInt r, Color c);
void draw_image_rect_simple(Image* img, int x, int y, int w, int h, Color c);
void flip_image_h_inplace(Image* img);
void flip_image_v_inplace(Image* img);
void copy_image(Image src, RectangleInt r, Image* dst, v2i offset);
void image_combine(Image src, RectangleInt r, Image* dst, v2i offset);
void image_ensure_max_size(Image* img, int max_size);
RectangleInt get_image_rect(Image img);
Color* get_pixels(Image img);
void draw_image_line_tool(v2i start, RectangleInt tool_rect,
                          RectangleInt img_rect, int ls, int sep, bool corner,
                          bool end_corner, Color c, Image* out, v2i* off);
void draw_image_bucket_tool(Image img, int x, int y, int sw, int sh, Color c,
                            bool force, Image* out, v2i* off);
Image image_encode_layers(int nl, Image* layers);
void image_decode_layers(Image img, int* nl, Image* layers);
Image ensure_size_multiple_of(Image img, int mof);

RenderTexture2D clone_texture(RenderTexture2D img);
RenderTexture2D clone_texture_from_image(Image img);
RenderTexture2D crop_texture(RenderTexture2D img, RectangleInt region);
void texture_combine(RenderTexture2D src, RectangleInt r, RenderTexture2D* dst,
                     v2i offset);
void fill_texture_rect(RenderTexture* img, RectangleInt r, Color c);
void copy_texture(RenderTexture2D src, RectangleInt r, RenderTexture2D* dst,
                  v2i offset);

void flip_texture_v_inplace(RenderTexture2D* img);
void flip_texture_h_inplace(RenderTexture2D* img);
RenderTexture2D rotate_texture(RenderTexture2D img, int ccw);

void draw_rt_on_screen(RenderTexture2D rt, v2 pos);

RenderTexture2D make_thumbnail(Image img, int tx, int ty);
void draw_projection_on_target(Cam2D cam, Tex2D tTmp, v2i szImg, int mode,
                               Color c);
void draw_projection_on_target_pattern(Cam2D cam, Tex2D tTmp, v2i szImg,
                                       int mode, int pad, Color color);

void draw_tex(Texture2D tex);
void draw_tex2(Texture2D tex, Color c);
void draw_stretched(Texture src, RenderTexture2D dst, Color c);
void gaussian(int dir, Texture2D in, RenderTexture2D out);
Image invert_image_v(Image img);
// void render_sidepanel(Image* out, Image buffer, Sim* sim, pindef_t* pdef);

void export_texture(Texture2D tex, const char* fname);

Image gen_thumbnail(int nl, Image* layers, int w, int h);

/*
 * grayscale with distance to edge
 * dist_type = 0 --> max x,y distance
 * dist_type = 1 --> L2 distance
 */
void project_with_dist(Cam2D cam, Texture2D img, int dist_type);
void project_regular(Cam2D cam, Texture2D img);
void naive_bokeh(Texture src);

void save_img_u8(int w, int h, u8* data, const char* fname);
void save_img_f32(int w, int h, float* data, float vmin, float vmax,
                  const char* fname);
void image_remove_blacks(Image* img);

#if defined(__cplusplus)
}
#endif

#endif
