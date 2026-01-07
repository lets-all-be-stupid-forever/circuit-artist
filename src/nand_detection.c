#include "nand_detection.h"

#include "assert.h"
#include "profiler.h"
#include "stb_ds.h"
#include "stdbool.h"

static inline int mini(int a, int b) { return a < b ? a : b; }
static inline int maxi(int a, int b) { return a > b ? a : b; }

typedef struct {
  int pw;
  int ph;
  int ox;
  int oy;
  // interface Wire coordinates
  int w1x;
  int w1y;
  int w2x;
  int w2y;
  int w3x;
  int w3y;
  int data[20];
} pattern_def_t;

// NAND patterns
static pattern_def_t pd_list[] = {
    {.ox = 1,  // Right
     .oy = 1,
     .pw = 4,
     .ph = 5,
     .w1x = -1,
     .w1y = 1,
     .w2x = -1,
     .w2y = 3,
     .w3x = 4,
     .w3y = 2,
     .data =
         {
             0, 0, 0, -1,  // row0
             0, 1, 0, 0,   // row1
             0, 0, 1, 0,   // row2
             0, 1, 0, 0,   // row3
             0, 0, 0, -1,  // row4
         }},
    {.ox = 2,  // Left
     .oy = 1,
     .pw = 4,
     .ph = 5,
     .w1x = 4,
     .w1y = 1,
     .w2x = 4,
     .w2y = 3,
     .w3x = -1,
     .w3y = 2,
     .data =
         {
             -1, 0, 0, 0,  // row0
             0,  0, 1, 0,  // row1
             0,  1, 0, 0,  // row2
             0,  0, 1, 0,  // row3
             -1, 0, 0, 0,  // row4
         }},
    {.ox = 2,  // Up
     .oy = 1,
     .pw = 5,
     .ph = 4,
     .w1x = 1,
     .w1y = 4,
     .w2x = 3,
     .w2y = 4,
     .w3x = 2,
     .w3y = -1,
     .data =
         {
             -1, 0, 0, 0, -1,  // row0
             0,  0, 1, 0, 0,   // row1
             0,  1, 0, 1, 0,   // row2
             0,  0, 0, 0, 0,   // row3
         }},
    {.ox = 1,  // Down
     .oy = 1,
     .pw = 5,
     .ph = 4,
     .w1x = 1,
     .w1y = -1,
     .w2x = 3,
     .w2y = -1,
     .w3x = 2,
     .w3y = 4,
     .data =
         {
             0,  0, 0, 0, 0,   // row0
             0,  1, 0, 1, 0,   // row1
             0,  0, 1, 0, 0,   // row2
             -1, 0, 0, 0, -1,  // row3
         }},
};

void find_nands(int w, int h, u8* img, NandLoc** out_nands) {
  profiler_tic_single("find_nand");
  int nn = 0;
  NandLoc* nands = 0;  // malloc(3 * cap * sizeof(int));
  int* cand = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      int b0 = img[idx];
      int b1 = y > 0 ? img[idx - w] : 0;      // up
      int b2 = x < w - 1 ? img[idx + 1] : 0;  // right
      int b3 = y < h - 1 ? img[idx + w] : 0;  // bot
      int b4 = x > 0 ? img[idx - 1] : 0;      // left
      int b = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3) | (b4 << 4);
      if (b == 1) {
        arrput(cand, idx);
      }
    }
  }

  for (int ip = 0; ip < 4; ip++) {
    pattern_def_t pd = pd_list[ip];
    int xmin = mini(0, mini(pd.w1x, mini(pd.w2x, pd.w3x)));
    int ymin = mini(0, mini(pd.w1y, mini(pd.w2y, pd.w3y)));
    int xmax = maxi(pd.pw, maxi(pd.w1x, maxi(pd.w2x, pd.w3x)));
    int ymax = maxi(pd.ph, maxi(pd.w1y, maxi(pd.w2y, pd.w3y)));
    for (int i = 0; i < arrlen(cand); i++) {
      int idx = cand[i];
      int x = idx % w;
      int y = idx / w;
      x = x - pd.ox;
      y = y - pd.oy;
      // if (x + xmin < 0 || y + ymin < 0 || x + xmax > w || y + ymax > h) {
      //   continue;
      // }
      bool ok = true;
      for (int xx = 0; xx < pd.pw; xx++) {
        for (int yy = 0; yy < pd.ph; yy++) {
          int p = pd.data[yy * pd.pw + xx];
          int v = 0;
          if (((y + yy) < 0) || (y + yy >= h) || (x + xx < 0) ||
              (x + xx >= w)) {
            v = 0;
          } else {
            v = img[(y + yy) * w + (x + xx)];
          }
          if (v != p && p != -1) {
            ok = false;
            break;
          }
        }
        if (!ok) break;
      }

      if (ok) {
        int x0 = x + pd.w1x;
        int y0 = y + pd.w1y;
        int x1 = x + pd.w2x;
        int y1 = y + pd.w2y;
        int x2 = x + pd.w3x;
        int y2 = y + pd.w3y;
        /* Checks if sockets and drivers fall inside the image */
        if ((x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) &&
            (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h) &&
            (x2 >= 0 && x2 < w && y2 >= 0 && y2 < h)) {
          NandLoc loc = {
              y0 * w + x0,  // first input
              y1 * w + x1,  // second input
              y2 * w + x2,  // output
          };
          arrput(nands, loc);
          nn++;
        }
      }
    }
  }
  arrfree(cand);
  *out_nands = nands;
  profiler_tac_single("find_nand");
}

void remove_nand_pixels(int w, u8* img, NandLoc* nands) {
  int nn = arrlen(nands);
  for (int i = 0; i < nn; i++) {
    int idx0 = nands[i].s1;
    int idx1 = nands[i].s2;
    int idx2 = nands[i].d;
    int x0 = idx0 % w;
    int y0 = idx0 / w;
    int x1 = idx1 % w;
    int y1 = idx1 / w;
    int x2 = idx2 % w;
    int y2 = idx2 / w;
    if (x0 == x1) {
      if (x2 > x1) {
        x2 -= 2;
        x0 += 2;
        x1 += 2;
      } else {
        x2 += 2;
        x0 -= 2;
        x1 -= 2;
      }
    } else {
      if (y2 > y1) {
        y2 -= 2;
        y0 += 2;
        y1 += 2;
      } else {
        y2 += 2;
        y0 -= 2;
        y1 -= 2;
      }
    }
    img[y0 * w + x0] = 0;
    img[y1 * w + x1] = 0;
    img[y2 * w + x2] = 0;
  }
}
