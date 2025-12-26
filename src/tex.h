#ifndef CA_TEX_H
#define CA_TEX_H
#include "img.h"

typedef struct {
  int tick;
  float slack;
  float utime;
  float glow_dt;
} Times;

/* Helper reference-counted texture buffer used for ARENA-like use.
 * The idea is to be able to re-use memory easily without worrying about memory
 * management during the pipeline.
 * I'll try to move all my gpu-based rendering stuff here.
 * */
typedef struct Tex {
  RenderTexture rt;
  int refc; /* uses = 0 --> Free, uses>0 --> being used */
  int uses;
  bool borrowed;
  int w; /* buffer Width */
  int h; /* buffer height */
  struct Tex* next;
} Tex;

/* Returns an available buffer with the given size */
Tex* texnew(int w, int h);

/* Returns a Tex object that doesnt own its memory */
Tex* texborrow(RenderTexture2D rt);

/* Returns an available buffer with same size as input */
Tex* texnewlike(Tex* t);

/* Releases reference of the buffer */
void texdel(Tex* t);

/* Copies the source into target, assumes same size */
void texdraw(Tex* dst, Tex* src);
void texdraw2(RenderTexture dst, RenderTexture src);

/* Gaussian smoothing */
Tex* texgauss(Tex* t);
Tex* texgauss2(Tex* t);

/* Projects source buffer into dst buffer using camera */
void texproj(Tex* src, Cam2D cam, Tex* dst);

/* Clean unused textures with reference 0 */
void texcleanup();

/* Fills buffer with color (ClearBackground)*/
void texclear(Tex* t, Color c);

// x := a *x + b *y + c
Tex* texaffine(Tex* x, Tex* y, float a, float b, float c);

void texmapcircuitlight(Texture2D circuit, Tex* pmap, Texture dmap,
                        int error_mode, Times times, Tex** circ, Tex** light);

/* Combines background and bloom */
Tex* texbloomcombine(Tex* base, Tex* bloom);

/* Accumulates light using mask */
Tex* texupdatelight(Tex* acc, Tex* l1, Tex* m1);

/* Needs circuit size as input */
void texdrawboard(Tex* t, Cam2D cam, int cw, int ch, Color c);

/* Draws clock for rewind mouse feature.
 * The mouse coordinates are relative to the center of the texture */
void texclock(Tex* t, float mx, float my);

#endif
