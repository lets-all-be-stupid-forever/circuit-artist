#ifndef IMG_H
#define IMG_H
#include "defs.h"
#include "rect_int.h"

#if defined(__cplusplus)
extern "C" {
#endif
// Thin wrapper over GenImage of raylib. The idea is that eventually the images
// will be forced to be in a specific format (like a 128-sized palette or
// something.)
Image GenImageSimple(int w, int h);

// This wrapper over GenImageColor.
Image GenImageFilled(int w, int h, Color v);

// Duplicates an image.
Image CloneImage(Image img);

// Crops an image. Returns a new image.
Image CropImage(Image img, RectangleInt region);

// Rotates an image. Returns a new image.
Image RotateImage(Image img, int ccw);

// Fills image with a color.
void FillImage(Image* img, Color v);

// Fills a subset of an image with a color.
void FillImageRect(Image* img, RectangleInt r, Color c);

// Draws a rectangle in an image.
void DrawImageRectSimple(Image* img, int x, int y, int w, int h, Color c);

//  Flips an image horizontally inplace.
void FlipImageHInplace(Image* img);

//  Flips an image vertically inplace.
void FlipImageVInplace(Image* img);

// Copies a subset of an image in another image.
void CopyImage(Image src, RectangleInt r, Image* dst, Vector2Int offset);

// A special version of image concatenation that treats black pixels as blank
// pixels (full transparency). Also makes sure there's no actual black in the
// image but BLANKS.
void ImageCombine(Image src, RectangleInt r, Image* dst, Vector2Int offset);
void ImageCombine2(Image src, RectangleInt r, Image* dst, Vector2Int offset);

// Replaces Blacks in images by BLANKs inplace.
// Also remove weird transparent pixels.
// Usually called when an image is imported.
void ImageRemoveBlacks(Image* img);

// Replaces BLANKs in an image by BLACKs inplace.
// Usually called when an image is exported.
void ImageAddBlacks(Image img);

// Ensures an image is smaller than a given size. (defined by a macro)
void ImageEnsureMaxSize(Image* img, int max_size);

// Returns the RectangleInt containing the whole image.
RectangleInt GetImageRect(Image img);

// Returns a pointer to the casted image data.
// It will crash if the image format is not R8G8B8A8.
Color* GetPixels(Image img);

// Algorithm for drawing the "Line Tool".
//
// It draws straight lines with the caveats:
// (i) it can draw multiple parallel lines at the same time via the "ls" (or
// line size) parameter.
// (ii) when drawing multiple lines, it can also adjust beginning and end tips
// of the lines so that it makes corners (when user presses shift/ctrl). This
// feature is useful for creating wire corners or connecting a list of wires at
// the same time.
//
// It creates a new image `out` which becomes ownership of the caller.
//
// The `start` parameter is where  the user first clicked during the use of the
// tool. It is used to define where the line "starts" from. The `tool_rect`
// parameter contains the rectangle generated by starting and end mouse points.
//
// `corner` is a flag for the corner at the beginning tip of the wire and
// `end_corner` is a flag for corenr at the end tip of the lines.
//
// `sep` is the separation between consecutive lines.
//
// The generated image `out` is a subset of the full image . The full image
// size is defined by `img_rect`,  and the offset of the subimage within the
// full image is given by the `off` output.
void DrawImageLineTool(Vector2Int start, RectangleInt tool_rect,
                       RectangleInt img_rect, int ls, int sep, bool corner,
                       bool end_corner, Color c, Image* out, Vector2Int* off);

// Algorithm for the "Bucket Tool".
//
// `img` is the original image before the tool is applied.
// `x`, `y`, `sw` and `sh` define the region where the bucket should be applied
// to with the color `c`.
//
// The `out` image contains a sub-image of the original image (positined by
// `off` offset) containing the new pixels after modification. The final result
// is the combination of the initial image with this new image.
//
// Note that it doesnt modify the image inplace directly.
//
// It's up to the caller to take ownership of the generated `out` image.
void DrawImageBucketTool(Image img, int x, int y, int sw, int sh, Color c,
                         Image* out, Vector2Int* off);

RenderTexture2D CloneTexture(RenderTexture2D img);
RenderTexture2D CloneTextureFromImage(Image img);
RenderTexture2D CropTexture(RenderTexture2D img, RectangleInt region);
void TextureCombine(RenderTexture2D src, RectangleInt r, RenderTexture2D* dst,
                    Vector2Int offset);
void FillTextureRect(RenderTexture* img, RectangleInt r, Color c);
void CopyTexture(RenderTexture2D src, RectangleInt r, RenderTexture2D* dst,
                 Vector2Int offset);

void FlipTextureVInplace(RenderTexture2D* img);
void FlipTextureHInplace(RenderTexture2D* img);
RenderTexture2D RotateTexture(RenderTexture2D img, int ccw);

void draw_rt_on_screen(RenderTexture2D rt, Vector2 pos);
void draw_main_img(int mode, Texture2D wire_tpl, RenderTexture2D img,
                   Texture2D tx, Texture2D ty, Texture2D ts, int simu_state,
                   RenderTexture2D sel, float sel_off_x, float sel_off_y,
                   RenderTexture2D tool, float tool_x, float tool_y, float cx,
                   float cy, float cs, RenderTexture2D* tmp,
                   RenderTexture2D* target);
void draw_rect(float x, float y, float w, int h, Color c);

#if defined(__cplusplus)
}
#endif

#endif
