#include "sim.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "profiler.h"

#define ALPHA_OFF 50
#define ALPHA_ON 255

static int MAX_CIRCUIT_UPDATE_COUNT = 100;
//////////////////
// Compile-time crash
//////////////////
static const char CRASH_REASON_MULTIPLE_GATE_CIRCUIT[] =
    "Circuits with multiple input gates.";
static const char CRASH_REASON_MISSING_INPUTS[] =
    "NANDs must have its 2 inputs filled";
static const char CRASH_REASON_MISSING_OUTPUT[] =
    "NANDs must be connected to an output wire.";

//////////////////
// Runtime crash
//////////////////
static const char CRASH_REASON_TOO_MANY_UPDATES[] =
    "Circuit updated too many times (infinite loop?).";

static void MakeNandLut(int* nand_lut);
static void SimQueueInitialInputEvents(Sim* s);

void SimInitCompVisu(Sim* s);
void UpdateStateTexture(Sim* s);

static inline void SimQueueEvent(Sim* s, int j, int vj) {
  if (s->queued_at[j] != -1) {
    // Here, we avoid queueing twice and only update the active queued
    // place.
    int kk = s->queued_at[j];
    s->ev[2 * kk + 0] = j;
    s->ev[2 * kk + 1] = vj;
  } else {
    s->num_updates_last_simulate++;
    s->queued_at[j] = s->ne;
    s->ev[2 * s->ne + 0] = j;
    s->ev[2 * s->ne + 1] = vj;
    s->ne++;
  }
}

// Union-find Find
static inline int UfFind(int* c, int a) {
  int root = a;
  while (c[root] != root) {
    root = c[root];
  }
  while (c[a] != root) {
    int par = c[a];
    c[a] = root;
    a = par;
  }
  return root;
}

// Union-find Union
static inline void UfUnion(int* c, int* r, int a, int b) {
  int ca = UfFind(c, a);
  int cb = UfFind(c, b);
  if (ca == cb) {
    return;
  }
  if (r[ca] < r[cb]) {
    c[ca] = cb;
  } else {
    c[cb] = ca;
    if (r[ca] == r[cb]) {
      r[ca] += 1;
    }
  }
}

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
} PatternDef;

PatternDef pd_list[] = {
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

static void FindNands(int w, int h, int* img, int* ones, int* num_ones,
                      int* nnand, int* nands, int* num_nand_pixels,
                      int** nand_pixels) {
  int no = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      if (img[idx] == 1) {
        ones[no++] = idx;
      }
    }
  }
  int npixels_size = 0;
  int npixels_maxsize = 100;
  int* npixels = malloc(npixels_maxsize * sizeof(int));
  int nn = 0;
  for (int ip = 0; ip < 4; ip++) {
    PatternDef pd = pd_list[ip];
    int xmin = MinInt(0, MinInt(pd.w1x, MinInt(pd.w2x, pd.w3x)));
    int ymin = MinInt(0, MinInt(pd.w1y, MinInt(pd.w2y, pd.w3y)));
    int xmax = MaxInt(pd.pw, MaxInt(pd.w1x, MaxInt(pd.w2x, pd.w3x)));
    int ymax = MaxInt(pd.ph, MaxInt(pd.w1y, MaxInt(pd.w2y, pd.w3y)));
    for (int i = 0; i < no; i++) {
      int idx = ones[i];
      int x = idx % w;
      int y = idx / w;
      x = x - pd.ox;
      y = y - pd.oy;

      if (x + xmin < 0 || y + ymin < 0 || x + xmax > w || y + ymax > h) {
        continue;
      }
      bool ok = true;
      for (int xx = 0; xx < pd.pw; xx++) {
        for (int yy = 0; yy < pd.ph; yy++) {
          int p = pd.data[yy * pd.pw + xx];
          if (img[(y + yy) * w + (x + xx)] != p && p != -1) {
            ok = false;
            break;
          }
        }
        if (!ok) break;
      }

      if (ok) {
        nands[3 * nn + 0] = (y + pd.w1y) * w + (x + pd.w1x);  // first input
        nands[3 * nn + 1] = (y + pd.w2y) * w + (x + pd.w2x);  // second input
        nands[3 * nn + 2] = (y + pd.w3y) * w + (x + pd.w3x);  // output
        nn++;
        // Now I disable the pixels...
        for (int xx = 0; xx < pd.pw; xx++) {
          for (int yy = 0; yy < pd.ph; yy++) {
            int p = pd.data[yy * pd.pw + xx];
            if (p == 1) {
              npixels[npixels_size++] = x + xx;
              npixels[npixels_size++] = y + yy;
              if (npixels_size > npixels_maxsize - 3) {
                npixels_maxsize = 2 * npixels_maxsize;
                npixels = realloc(npixels, npixels_maxsize * sizeof(int));
              }
              img[(y + yy) * w + (x + xx)] = 2;
            }
          }
        }
      }
    }
  }

  *nnand = nn;
  int k = 0;
  for (int i = 0; i < no; i++) {
    int idx = ones[i];
    if (img[idx] == 1) {
      ones[k++] = idx;
    }
  }
  *num_ones = k;
  *nand_pixels = npixels;
  *num_nand_pixels = npixels_size / 2;
}

static int ParseComponents(Image src_img, int w, int h, int* img, int no,
                           int* ones, int* out_comp, int* comp_off,
                           int* comp_pixels) {
  int* comp = malloc(w * h * 2 * sizeof(int));
  int* r = malloc(w * h * 2 * sizeof(int));
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      out_comp[y * w + x] = 0;
    }
  }

  // Finds ones and assign components.
  for (int i = 0; i < no; i++) {
    int idx = ones[i];
    // each component is assigned the identity
    comp[2 * idx + 0] =
        2 * idx + 0;  // when idx is 0, this is assigned the 0 component.
    comp[2 * idx + 1] = 2 * idx + 1;
    r[2 * (idx) + 0] = 0;
    r[2 * (idx) + 1] = 0;
    // odd indexes are vertical components, even are horizontal
  }
  // Connects horizontal [2idx + 0] and vertical [2idx + 1] edges.
  // checks only Bottom/right pixel
  for (int i = 0; i < no; i++) {
    int idx = ones[i];
    int x = idx % w;
    int y = idx / w;

    // bottom index
    int bidx = (y + 1) * w + x;
    // right index
    int ridx = (y)*w + (x + 1);
    // vertical edge
    if (y < h - 1 && img[bidx] == 1) {
      UfUnion(comp, r, 2 * idx + 1, 2 * bidx + 1);
    }
    // horizontal
    if (x < w - 1 && img[ridx] == 1) {
      UfUnion(comp, r, 2 * idx + 0, 2 * ridx + 0);
    }
    // vert-hori
    // TODO this might lead to issues in the borders
    if (x < w - 1 && y < h - 1 && x > 0 && y > 0) {
      int a = img[idx + 1] == 1;
      int b = img[idx + w] == 1;
      int c = img[idx - w] == 1;
      int d = img[idx - 1] == 1;

      // not a cross
      bool iscross = a && b && c && d;
      if (!iscross) {
        UfUnion(comp, r, 2 * idx + 0, 2 * idx + 1);
      }
    } else {
      // In the border, we always want to merge
      UfUnion(comp, r, 2 * idx + 0, 2 * idx + 1);
    }
  }
  int* m = malloc(2 * h * w * sizeof(int));
  for (int i = 0; i < no; i++) {
    int idx = ones[i];
    // gets component of the pixel
    int c = UfFind(comp, 2 * idx + 0);
    m[c] = 0;
  }

  int nc = 1;  // starts with the background component
  comp_off[0] = 0;
  Color* pixels = GetPixels(src_img);
  // For each white pixel (ie 1)
  for (int i = 0; i < no; i++) {
    int idx = ones[i];
    // gets component of the pixel
    // This is the component of the horizontal component
    // We might also want the component of the vertical one...
    int ch = UfFind(comp, 2 * idx + 0);
    int cv = UfFind(comp, 2 * idx + 1);
    int c = ch;
    if (ch != cv) {
      // This is a cross-section scenario.
      // Here we employ a more strict strategy to decide which one should be on
      // top. Instead of systematically picking horizontal, we can give
      // preference to the one having a neighbor of same color.
      // For example, if intersection is red, and we have a red pixel in the
      // vertical, we give this pixel the vertical component.
      // IF all  colors are equal, we keep the horizontal.
      // Since it's a crossing, we already know it's not in the border.
      Color center = pixels[idx];
      int nv = 0;
      int nh = 0;
      nv += COLOR_EQ(pixels[idx - w], center);
      nv += COLOR_EQ(pixels[idx + w], center);
      nh += COLOR_EQ(pixels[idx - 1], center);
      nh += COLOR_EQ(pixels[idx + 1], center);
      if (nv > nh) {
        c = cv;
      }
    }

    int k;
    // checks if its component is not already on the mapping
    if (m[c] != 0) {
      // If it's on the map, retrieves it.
      k = m[c];
    } else {
      // if component is not on the mapping (ie, it's a new component), then
      // creates a new component number and save it in the component hashmap
      comp_off[nc] = 0;
      k = nc++;
      m[c] = k;
    }
    out_comp[idx] = k;
    comp_off[k]++;
  }
  int* cnt = malloc(nc * sizeof(int));
  for (int i = 1; i < nc; i++) {
    cnt[i] = 0;
    comp_off[i] += comp_off[i - 1];
  }

  for (int i = 0; i < no; i++) {
    int idx = ones[i];
    int k = out_comp[idx];
    assert(k > 0 && k < nc);
    int c = cnt[k]++;
    int off = comp_off[k - 1];
    assert(off + c < w * h);
    comp_pixels[off + c] = idx;
  }

  free(m);
  free(r);
  free(cnt);
  free(comp);
  return nc;
}

ParsedImage ParseImage(Image image) {
  double start = GetTime();
  Color* colors = GetPixels(image);
  ParsedImage pi = {0};
  int w = image.width;
  int h = image.height;
  pi.width = w;
  pi.height = h;
  pi.image = malloc(w * h * sizeof(int));
  pi.comp = malloc(w * h * sizeof(int));
  pi.comp_off = malloc(w * h * sizeof(int));
  pi.comp_pixels = malloc(w * h * sizeof(int));
  pi.num_nands = 0;
  pi.nands = malloc(w * h * 2 * sizeof(int));
  pi.num_nand_pixels = 0;
  pi.nand_pixels = NULL;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      pi.image[y * w + x] = colors[y * w + x].a != 0;
    }
  }
  int* ones = malloc(w * h * sizeof(int));
  int num_ones = -1;
  FindNands(w, h, pi.image, ones, &num_ones, &pi.num_nands, pi.nands,
            &pi.num_nand_pixels, &pi.nand_pixels);
  pi.nc = ParseComponents(image, w, h, pi.image, num_ones, ones, pi.comp,
                          pi.comp_off, pi.comp_pixels);
  double end = GetTime();
  pi.time_parsing = 1000 * (end - start);
  pi.original_image = image;
  free(ones);
  return pi;
}

void UnloadParsedImage(ParsedImage pi) {
  if (pi.nand_pixels) free(pi.nand_pixels);
  free(pi.comp_off);
  free(pi.comp_pixels);
  free(pi.image);
  free(pi.comp);
  free(pi.nands);
}

void SimFindNearestPixelToToggle(Sim* s, int tol, float fx, float fy, int* px,
                                 int* py) {
  if (!s->ok_creation) {
    *px = -1;
    *py = -1;
    return;
  }
  assert(tol >= 0);
  int w = s->pi.width;
  int h = s->pi.height;
  int x = (int)floorf(fx);
  int y = (int)floorf(fy);
  int x0 = MaxInt(x - tol, 0);
  int x1 = MinInt(x + tol, w - 1);
  int y0 = MaxInt(y - tol, 0);
  int y1 = MinInt(y + tol, h - 1);

  float dmin = 10000;
  int xmin = -1;
  int ymin = -1;
  for (int yy = y0; yy <= y1; yy++) {
    for (int xx = x0; xx <= x1; xx++) {
      // center of pixel
      float cx = xx + 0.5f;
      float cy = yy + 0.5f;
      float dx = cx - fx;
      float dy = cy - fy;
      float sdist = dx * dx + dy * dy;
      int idx = yy * w + xx;
      int c = s->pi.comp[idx];
      if (c != 0 && sdist < dmin) {
        dmin = sdist;
        xmin = xx;
        ymin = yy;
      }
    }
  }
  if (xmin != -1) {
    *px = xmin;
    *py = ymin;
  } else {
    *px = -1;
    *py = -1;
  }
}

int SimGetWireAtPixel(Sim* s, int x, int y) {
  int w = s->pi.width;
  if (x < 0 || x >= s->pi.width || y < 0 || y >= s->pi.height) {
    return 0;
  }
  int idx = y * w + x;
  int c = s->pi.comp[idx];
  return c;
}

int SimGetWireValue(Sim* s, int x, int y) {
  if (x < 0 || x >= s->pi.width || y < 0 || y >= s->pi.height) {
    return 0;
  }
  int w = s->pi.width;
  int idx = y * w + x;
  int c = s->pi.comp[idx];
  if (c > 0) {
    return s->state[c];
  } else {
    return -2;
  }
}

void SimTogglePixel(Sim* s, int x, int y) {
  int w = s->pi.width;
  int idx = y * w + x;
  int c = s->pi.comp[idx];
  // Don't want to simulate background component
  if (c != 0 && s->ok_creation) {
    SimQueueEvent(s, c, !s->state[c]);
  }
}

static void SimCalcFanout(int nc, const int* topology, int* nfo, int* fo) {
  // maybe 2 passes?
  // 1st counting, 2nd filling values

  int* tc = malloc(nc * sizeof(int));

  nfo[0] = 0;
  for (int ic = 0; ic < nc; ic++) {
    nfo[ic + 1] = 0;
    tc[ic] = 0;
  }

  for (int k = 0; k < nc; k++) {
    int k1 = topology[2 * k + 0];
    int k2 = topology[2 * k + 1];
    nfo[k1 + 1]++;
    nfo[k2 + 1]++;
  }

  for (int k = 0; k < nc; k++) {
    nfo[k + 1] = nfo[k + 1] + nfo[k];
  }

  for (int k = 0; k < nc; k++) {
    int k1 = topology[2 * k + 0];
    int k2 = topology[2 * k + 1];
    // k1->k
    // k2->k
    int off1 = nfo[k1];
    int off2 = nfo[k2];
    fo[off1 + tc[k1]++] = k;
    fo[off2 + tc[k2]++] = k;
    // component 0 is special: it connects to all inputs
    // It also connects to itsel
  }
  free(tc);
}

void SimLoad(Sim* s, ParsedImage pi, int necomps, ExtComp* ecomps,
             ComponentUpdateCallback update_cb, void* update_ctx) {
  double start = GetTime();
  *s = (Sim){0};
  s->update_cb = update_cb;
  s->update_ctx = update_ctx;
  s->pi = pi;
  s->nc = s->pi.nc;
  s->state = malloc(s->nc * sizeof(int));
  s->last_render_state = malloc(s->nc * sizeof(int));
  s->graph = malloc(2 * s->nc * sizeof(int));
  s->bugged_flag = malloc(s->nc * sizeof(int));
  s->nand_error_status = NULL;
  if (pi.num_nands > 0) {
    s->nand_error_status = malloc(pi.num_nands * sizeof(int));
  }
  s->max_events = s->nc;
  s->ne = 0;
  s->ne_swap = 0;
  s->ev = malloc(2 * s->nc * sizeof(int));
  s->ev_swap = malloc(2 * s->nc * sizeof(int));
  s->ok_creation = true;
  s->nfo = malloc((s->nc + 1) * sizeof(int));
  s->fo = malloc((2 * s->nc) * sizeof(int));
  s->update_count = malloc(s->nc * sizeof(int));
  s->queued_at = malloc(s->nc * sizeof(int));
  s->crash_reason = NULL;
  s->status = SIMU_STATUS_OK;
  s->necomps = necomps;
  if (s->necomps > 0) {
    s->ecomps = ecomps;
    s->ecomp_inputs = malloc(necomps * MAX_SLOTS * sizeof(int));
    s->ecomp_outputs = malloc(necomps * MAX_SLOTS * sizeof(int));
    for (int i = 0; i < necomps * MAX_SLOTS; i++) {
      s->ecomp_inputs[i] = BIT_UNDEFINED;
      s->ecomp_outputs[i] = BIT_UNDEFINED;
    }
  }

  MakeNandLut(s->nand_lut);

  // Makes all components connect to 0 (ie, all inputs by default)
  for (int i = 0; i < s->nc; i++) {
    s->graph[2 * i + 0] = 0;
    s->graph[2 * i + 1] = 0;
    s->bugged_flag[i] = 0;
  }

  // initializes the graph.
  // For each nand, we connect N[2] to N[0] and N[1].
  // If connection already exists, an error is raised for that connection!
  for (int i = 0; i < pi.num_nands; i++) {
    int pa = pi.nands[3 * i + 0];  // first input
    int pb = pi.nands[3 * i + 1];  // second input
    int pc = pi.nands[3 * i + 2];  // output

    int ca = pi.comp[pa];
    int cb = pi.comp[pb];
    int cc = pi.comp[pc];

    // If it's not connected anywhere, ignore it.
    if (cc == 0) {
      s->nand_error_status[i] = NAND_STATUS_MISSING_OUTPUT;
      s->crash_reason = CRASH_REASON_MISSING_OUTPUT;
      s->status = SIMU_STATUS_NAND_MISSING_OUTPUT;
      s->ok_creation = false;
      continue;
    }
    if (ca == 0 || cb == 0) {
      s->nand_error_status[i] = NAND_STATUS_MISSING_INPUT;
      s->crash_reason = CRASH_REASON_MISSING_INPUTS;
      s->status = SIMU_STATUS_NAND_MISSING_INPUT;
      s->ok_creation = false;
      continue;
    }

    assert(ca < s->nc);
    assert(cb < s->nc);
    assert(cc < s->nc);
    assert(ca >= 0);
    assert(cb >= 0);
    assert(cc > 0);
    // The circuits where the output are connected to
    int prev_a = s->graph[2 * cc + 0];
    int prev_b = s->graph[2 * cc + 1];

    if ((prev_a != 0) || (prev_b != 0)) {
      // here, we have a connection that already exists!
      // This means that the output wire is buggy!
      s->ok_creation = false;
      s->crash_reason = CRASH_REASON_MULTIPLE_GATE_CIRCUIT;
      s->status = SIMU_STATUS_WIRE;
      s->bugged_flag[cc]++;
    } else {
      s->graph[2 * cc + 0] = ca;
      s->graph[2 * cc + 1] = cb;
    }
    s->nand_error_status[i] = NAND_STATUS_OK;
  }

  // If there are bugged circuits, no need for fanout calculation
  if (s->ok_creation) {
    SimCalcFanout(s->nc, s->graph, s->nfo, s->fo);
  }

  // Here I check if we don't have same wire with multiple component outputs
  for (int iec = 0; iec < s->necomps; iec++) {
    ExtComp* comp = &s->ecomps[iec];

    for (int i = 0; i < comp->no; i++) {
      int ox = comp->wires_out_x[i];
      int oy = comp->wires_out_y[i];
      int cc = SimGetWireAtPixel(s, ox, oy);
      // non-output: I dont care
      if (cc == 0) {
        continue;
      }
      int prev_a = s->graph[2 * cc + 0];
      int prev_b = s->graph[2 * cc + 1];
      if ((prev_a != 0) || (prev_b != 0)) {
        s->ok_creation = false;
        s->crash_reason = CRASH_REASON_MULTIPLE_GATE_CIRCUIT;
        s->status = SIMU_STATUS_WIRE;
        s->bugged_flag[cc]++;
      }
      s->graph[2 * cc + 0] = -iec - 1;
      s->graph[2 * cc + 1] = -i - 1;
    }
  }

  // Initializes all states as undefined.
  for (int i = 0; i < s->nc; i++) {
    if (s->bugged_flag[i]) {
      s->state[i] = BIT_BUGGED;
    } else {
      s->state[i] = BIT_UNDEFINED;
    }
    s->last_render_state[i] = -1;
  }

  SimInitCompVisu(s);
  UpdateStateTexture(s);
  double end = GetTime();
  s->time_creation = 1000 * (end - start);
  s->wire_has_changed = malloc(s->nc * sizeof(bool));
  s->wire_changed_stack = malloc(s->nc * sizeof(int));

  // Resets the update count of each gate
  // This takes time when there's no activity
  for (int i = 0; i < s->nc; i++) {
    s->wire_has_changed[i] = false;
    s->update_count[i] = 0;
    s->queued_at[i] = -1;  // -1 == not queued.
  }
  s->time_parsing = s->pi.time_parsing;
  if (s->ok_creation) {
    SimQueueInitialInputEvents(s);
  }
}

void SimQueueInitialInputEvents(Sim* s) {
  // step1: find all inputs.
  // step2: queue all values 0 for them.
  // the component 0 is special, we dont send event to it
  for (int i = 1; i < s->nc; i++) {
    int ca = s->graph[2 * i + 0];
    int cb = s->graph[2 * i + 1];
    if (ca == 0 && cb == 0) {
      SimQueueEvent(s, i, 0);
    }
  }
}

static void MakeNandLut(int* nand_lut) {
  // 9 values to setup
  // Off = 0
  // On = 1
  // X = 2 (undefined)
  // NAND(X,0) --> 1
  // NAND(X,1) --> X
  // NAND(X,X) --> X
  // NAND(0,0) --> 1
  // NAND(1,0) --> 1
  // NAND(1,1) --> 0

  int a0 = 0;
  int a1 = 1;
  int ax = 2;
  int b0 = a0 << 2;
  int b1 = a1 << 2;
  int bx = ax << 2;

  nand_lut[b0 + a0] = a1;
  nand_lut[b0 + a1] = a1;
  nand_lut[b0 + ax] = a1;
  nand_lut[b1 + a0] = a1;
  nand_lut[b1 + a1] = a0;
  nand_lut[b1 + ax] = ax;
  nand_lut[bx + a0] = a1;
  nand_lut[bx + a1] = ax;
  nand_lut[bx + ax] = ax;
}

static void SimCrashSimulation(Sim* s) {
  s->ok_creation = false;
  for (int i = 0; i < s->nc; i++) {
    s->state[i] = BIT_UNDEFINED;
    if (s->update_count[i] > MAX_CIRCUIT_UPDATE_COUNT / 2) {
      s->state[i] = BIT_BUGGED;
    }
  }
}

static void SimSwapEvent(Sim* s) {
  // Resets queue flags
  for (int i2 = 0; i2 < s->ne; i2++) {
    int k = s->ev[2 * i2 + 0];  // the gate
    s->queued_at[k] = -1;
  }
  s->ne_swap = s->ne;
  s->ne = 0;
  int* tmp = s->ev;
  s->ev = s->ev_swap;
  s->ev_swap = tmp;
}

void SimSimulate(Sim* s) {
  // No change in circuit if there was parsing errors
  if (s->ok_creation == false) {
    return;
  }
  double start = GetTime();

  // int n1 = 0;
  // int n2 = 0;
  // int* e1 = s->e1;
  // int* e2 = s->e2;
  // // Initialize the event queue.
  // for (int i = 0; i < s->ne; i++) {
  //   int k = s->events[2 * i + 0];
  //   int v = s->events[2 * i + 1];
  //   e1[2 * n1 + 0] = k;
  //   e1[2 * n1 + 1] = v;
  //   n1++;
  // }
  // e1 --> queued for processing now.
  // e2 --> next processing

  int num_updated = 0;
  // Resets events
  s->num_updates_last_simulate = 0;
  // int num_comp_updates = 0;
  while (true) {
    SimSwapEvent(s);
    for (int i1 = 0; i1 < s->ne_swap; i1++) {
      // k is the ID of the component to be updated
      int k = s->ev_swap[2 * i1 + 0];
      // v is the new value of this component (ie output)
      int v = s->ev_swap[2 * i1 + 1];
      if (s->state[k] == v) {
        continue;
      }
      // Actual wire Update has happened
      // Attention: wire update is differetn from nand update
      s->update_count[k]++;
      // the purpose of wire_has_changed right now is to set update to 0
      if (!s->wire_has_changed[k]) {
        s->wire_has_changed[k] = true;
        s->wire_changed_stack[num_updated++] = k;
      }
      if (s->update_count[k] > MAX_CIRCUIT_UPDATE_COUNT) {
        SimCrashSimulation(s);
        s->status = SIMU_STATUS_LOOP;
        s->crash_reason = CRASH_REASON_TOO_MANY_UPDATES;
        return;
      }
      s->state[k] = v;
      int nf = s->nfo[k + 1] - s->nfo[k];
      int off = s->nfo[k];
      for (int i = 0; i < nf; i++) {
        int j = s->fo[off + i];
        // int prev_vj = s->state[j];
        // Updates j
        int i1 = s->graph[2 * j + 0];
        int i2 = s->graph[2 * j + 1];
        // cache horror!
        int v1 = s->state[i1];
        int v2 = s->state[i2];
        int next_vj = s->nand_lut[(v1 << 2) + v2];
        SimQueueEvent(s, j, next_vj);
      }
    }

    // Trying to simulate external components
    if (s->ne == 0) {
      // Pseudo-code:
      // int nextInputs[MAX_SLOTS];
      // int prevInputs[MAX_SLOTS];
      // int prevOutputs[MAX_SLOTS];
      // int nextOutputs[MAX_SLOTS];
      // for (every comp in s->comps) {
      //   CollectInputs(s, comp, nextInputs);
      //   GetPrevInput(comp, prevInputs);
      //   // Comp can be dirty from "outside" events
      //   if (prevInput != nextInput || GetDirtyFlag(comp)) {
      //     // comp contains its state
      //     UpdateComponent(comp, prevInputs, nextInputs, nextOutput);
      //     UpdatePrevInput(comp, nextInputs);
      //     ResetDirtyFlag(comp);
      //     CollectOutput(s, comp, prevOutput);
      //     for (ibit in len(prevOutput)) {
      //       int wire = GetOutputWire(comp, ibit);
      //       if (nextOutput[i] != prevOutput[i]) {
      //         EnqueueWireEvent(s, wire, nextOutput[i]); <-- Note that during
      //         component update a wire can't change more than once!
      //       }
      //     }
      //   }
      // }
      int next_inputs[MAX_SLOTS];
      for (int iec = 0; iec < s->necomps; iec++) {
        int* next_outputs = &s->ecomp_outputs[iec * MAX_SLOTS];
        int* prev_inputs = &s->ecomp_inputs[iec * MAX_SLOTS];
        ExtComp* comp = &s->ecomps[iec];
        bool need_simu = comp->dirty;
        for (int i = 0; i < comp->ni; i++) {
          int ix = comp->wires_in_x[i];
          int iy = comp->wires_in_y[i];
          next_inputs[i] = SimGetWireValue(s, ix, iy);
          if (next_inputs[i] != prev_inputs[i]) {
            need_simu = true;
          }
        }
        if (need_simu) {
          if (s->update_cb) {
            s->update_cb(s->update_ctx, iec, prev_inputs, next_inputs,
                         next_outputs);
          }
          // Updates previous inputs
          for (int i = 0; i < comp->ni; i++) {
            prev_inputs[i] = next_inputs[i];
          }
          comp->dirty = false;
          for (int i = 0; i < comp->no; i++) {
            int ox = comp->wires_out_x[i];
            int oy = comp->wires_out_y[i];
            int w = SimGetWireAtPixel(s, ox, oy);
            int prev_output = s->state[w];
            if (prev_output != next_outputs[i] && w != 0) {
              SimQueueEvent(s, w, next_outputs[i]);
            }
          }
        }
      }
    }
    // stops when event queue is empty
    if (s->ne == 0) {
      break;
    }
  }

  for (int i = 0; i < num_updated; i++) {
    int w = s->wire_changed_stack[i];
    s->wire_has_changed[w] = false;
    s->update_count[w] = 0;
  }
  s->total_updates = s->num_updates_last_simulate + s->total_updates;
}

void SimUnload(Sim* s) {
  if (s->ecomps) {
    free(s->ecomp_inputs);
    free(s->ecomp_outputs);
  }
  for (int i = 0; i < 3; i++) {
    if (s->simulated[i].width > 0) {
      UnloadImage(s->simulated[i]);
    }
    if (s->t_comp_x.width > 0) {
      UnloadTexture(s->t_comp_x);
      UnloadTexture(s->t_comp_y);
    }
  }
  if (s->t_state.width > 0) {
    UnloadTexture(s->t_state);
  }
  if (s->state_buffer) {
    free(s->state_buffer);
  }
  if (s->nand_error_status) {
    free(s->nand_error_status);
  }
  free(s->wire_has_changed);
  free(s->wire_changed_stack);
  free(s->nfo);
  free(s->bugged_flag);
  free(s->fo);
  free(s->graph);
  free(s->state);
  free(s->ev);
  free(s->ev_swap);
  free(s->update_count);
  free(s->queued_at);
  *s = (Sim){0};
}

static inline Color ColorWithAlpha(Color c, int a) {
  return (Color){c.r, c.g, c.b, a};
}

void SimGenImage(Sim* s) {
  double start = GetTime();
  int w = s->pi.width;
  int h = s->pi.height;
  Color zero = GetColor(0x614540FF);
  Color one = GetColor(0xFA3410FF);
  Color nand = GetColor(0x945044FF);
  Color black = BLACK;
  Color undefined = MAGENTA;
  Color bugged = RED;
  ProfilerTic("SimGenGPU");
  UpdateStateTexture(s);
  ProfilerTac();
  s->time_gen_image = 1000 * (GetTime() - start);
}

int SimGetNumNands(Sim* s) { return s->pi.num_nands; }

int* SimGetComponentInputs(Sim* s, int icomp) {
  return &s->ecomp_inputs[icomp * MAX_SLOTS];
}

int* SimGetComponentOutputs(Sim* s, int icomp) {
  return &s->ecomp_outputs[icomp * MAX_SLOTS];
}

Color GetWireColor(int wire_value) {
  Color zero = GetColor(0x614540FF);
  Color one = GetColor(0xFA3410FF);
  Color undefined = MAGENTA;
  Color bugged = RED;
  Color lut[] = {
      zero,
      one,
      undefined,
      bugged,
  };
  return lut[wire_value];
}

Color GetSimuColorOnWire(Color c, int v) {
  Color undefined = MAGENTA;
  Color bugged = RED;
  if (v == BIT_0 || v == BIT_1) {
    int color_alpha = v == BIT_0 ? ALPHA_OFF : ALPHA_ON;
    c.a = color_alpha;
    return c;
  } else {
    if (v == BIT_UNDEFINED) {
      c = undefined;
    } else if (v == BIT_BUGGED) {
      c = bugged;
    }
  }
  return c;
}

void SimDispatchComponent(Sim* s, int icomp) {
  int* next_outputs = &s->ecomp_outputs[icomp * MAX_SLOTS];
  int* prev_inputs = &s->ecomp_inputs[icomp * MAX_SLOTS];
  int next_inputs[MAX_SLOTS];
  ExtComp* comp = &s->ecomps[icomp];
  for (int i = 0; i < comp->ni; i++) {
    int ix = comp->wires_in_x[i];
    int iy = comp->wires_in_y[i];
    next_inputs[i] = SimGetWireValue(s, ix, iy);
  }
  if (s->update_cb) {
    s->update_cb(s->update_ctx, icomp, prev_inputs, next_inputs, next_outputs);
  }

  for (int i = 0; i < comp->ni; i++) {
    prev_inputs[i] = next_inputs[i];
  }
  comp->dirty = false;
  for (int i = 0; i < comp->no; i++) {
    int ox = comp->wires_out_x[i];
    int oy = comp->wires_out_y[i];
    int w = SimGetWireAtPixel(s, ox, oy);
    int prev_output = s->state[w];
    if (prev_output != next_outputs[i] && w != 0) {
      SimQueueEvent(s, w, next_outputs[i]);
      // s->ev[2 * s->ne + 0] = w;
      // s->ev[2 * s->ne + 1] = next_outputs[i];
      // s->ne++;
    }
  }
}

void SimInitCompVisu(Sim* s) {
  // if (!s->ok_creation) {
  //   return;
  // }

  s->state_w = 1024;
  int state_size = s->nc + s->pi.num_nands;
  s->state_h = (state_size / 1024) + 2;
  s->state_buffer = malloc(s->state_w * s->state_h * sizeof(float));

  Image img_state = {.data = s->state_buffer,
                     .mipmaps = 1,
                     .height = s->state_h,
                     .width = s->state_w,
                     .format = PIXELFORMAT_UNCOMPRESSED_R32};
  s->t_state = LoadTextureFromImage(img_state);

  int w = s->pi.original_image.width;
  int h = s->pi.original_image.height;
  float* tmp_x = malloc(w * h * sizeof(float));
  float* tmp_y = malloc(w * h * sizeof(float));

  int sw = s->state_w;
  int sh = s->state_h;
  // how to do this?
  // isso ta na vdd na parsedimage
  int off = 0;  // l * w * h;
  int* comp = &s->pi.comp[off];
  // TODO: care with pixel order!
  int k = 0;
  for (int y = 0; y < h; y++) {
    int off = (h - 1 - y) * w;
    for (int x = 0; x < w; x++) {
      tmp_x[off + x] = (comp[k] % sw) / ((float)(sw - 1));
      tmp_y[off + x] = (comp[k] / sw) / ((float)(sh - 1));
      k++;
    }
  }

  // TODO: set components of nands.
  for (int i = 0; i < s->pi.num_nand_pixels; i++) {
    // int iz = s->pi.nand_pixels[3 * i + 2];
    // if (iz != l) continue;
    int ix = s->pi.nand_pixels[2 * i + 0];
    int iy = s->pi.nand_pixels[2 * i + 1];
    // int off = (h - 1 - iy) * w;
    int off = (h - 1 - iy) * w;
    int idx = off + ix;
    int nand_idx = i / 3;
    int c = nand_idx + s->nc;
    tmp_x[idx] = (c % sw) / ((float)(sw - 1));
    tmp_y[idx] = (c / sw) / ((float)(sh - 1));
    // TODO: Render nands separately
    // if (s->ok_creation) {
    //   tmp_x[idx] = 0.91;
    //   tmp_y[idx] = 0.91;
    // } else {
    //   switch (status) {
    //     case NAND_STATUS_OK:
    //       // pixels[idx] = undefined;
    //       tmp_x[idx] = 0.92;
    //       tmp_y[idx] = 0.92;
    //       break;
    //     case NAND_STATUS_MISSING_INPUT:
    //     case NAND_STATUS_MISSING_OUTPUT:
    //       // pixels[idx] = bugged;
    //       tmp_x[idx] = 0.93;
    //       tmp_y[idx] = 0.93;
    //       break;
    //   }
    // }
  }

  Image img_x = {.data = tmp_x,
                 .height = h,
                 .width = w,
                 .mipmaps = 1,
                 .format = PIXELFORMAT_UNCOMPRESSED_R32};
  Image img_y = {.data = tmp_y,
                 .height = h,
                 .width = w,
                 .mipmaps = 1,
                 .format = PIXELFORMAT_UNCOMPRESSED_R32};
  s->t_comp_x = LoadTextureFromImage(img_x);
  s->t_comp_y = LoadTextureFromImage(img_y);
  free(tmp_x);
  free(tmp_y);
}

void UpdateStateTexture(Sim* s) {
  // if (!s->ok_creation) {
  //   return;
  // }
  float lut[5];
  lut[BIT_0] = 0.3;
  lut[BIT_1] = 0.4;
  lut[BIT_UNDEFINED] = 0.2;
  lut[BIT_BUGGED] = 0.1;
  s->state_buffer[0] = 0.0;
  for (int c = 1; c < s->nc; c++) {
    int cs = s->state[c];
    s->state_buffer[c] = lut[cs];
  }

  for (int i = 0; i < s->pi.num_nands; i++) {
    NandStatusEnum status = s->nand_error_status[i];
    int idx = i + s->nc;
    switch (status) {
      case NAND_STATUS_OK: {
        s->state_buffer[idx] = 0.5;
        break;
      }
      case NAND_STATUS_MISSING_INPUT:
      case NAND_STATUS_MISSING_OUTPUT:
        s->state_buffer[idx] = lut[BIT_BUGGED];
        break;
    }
  }

  UpdateTexture(s->t_state, s->state_buffer);
}
