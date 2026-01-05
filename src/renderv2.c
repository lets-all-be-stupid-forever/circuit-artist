#include "renderv2.h"

#include "assert.h"
#include "limits.h"
#include "math.h"
#include "raymath.h"
#include "rlgl.h"
#include "shaders.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"
#include "union_find.h"

static void bucket_sort(int ng, int* g0, int* s0, int nb, int* bins, int* g2,
                        int* s2) {
  assert(nb <= 200);
  int* bucket[200];
  for (int i = 0; i < nb; i++) {
    bucket[i] = NULL;
  }
  for (int i = 0; i < ng; i++) {
    int g = g0[i];
    int s = s0[i];
    int k = nb - 1;
    for (int b = 0; b < nb - 1; b++) {
      if (s < bins[b]) {
        k = b;
        break;
      }
    }
    arrput(bucket[k], i);
  }
  int k = 0;
  for (int b = 0; b < nb; b++) {
    int n = arrlen(bucket[b]);
    for (int i = 0; i < n; i++) {
      int ig = bucket[b][i];
      g2[k] = g0[ig];
      s2[k] = s0[ig];
      k++;
    }
  }
  for (int i = 0; i < nb; i++) {
    arrfree(bucket[i]);
  }
}

/* A more fine but more complex algorithm for scheduling..
 * If the naive solution is not enough I implement this one.
 * Step1: Find dirty offset blocks.
 * These contain just the TASK offset, not blocks. It starts with dirty
 * tasks, and then alternate between clean and dirty. Example:
 *   off[0] = 5
 *   off[1] = 35
 *   off[2] = 50
 *   off[3] = 70
 * Means:
 *    - 5-35 ==> Dirty tasks
 *    - 50-70 ==> Dirty tasks
 * The size is always 2k+1, where k is the number of dirty consecutive
 * blocks.
 * The gaps, or clean tasks are never empty.
 * An empty case would have
 *   off[0] = NUM_TASKS or
 *   off[0] =  wire_block[-1].off + wire_block[-1].size
 * After this step, we handle directly with tasks and not blocks anymore.
 * We also keep track of:
 *   (i) the total number of tasks to execute (ie dirty)
 *   (ii) The number of task segments.
 * We also sort the "gaps" by rough size by power of two in B bins. The gaps
 * that are too big are not accounted for.
 *
 * Gap block `k` is between dirty block `k` and dirty block `k+1`.
 * Step2: Merging
 * Now we merge consecutive blocks, until the stop condition is met.
 * Stop condition is that we want T tasks per call in average.
 * So, we want total_tasks >= num_segs * T.
 * We merge segments starting from small gaps and
 *
 * However, there's no reason to keep a gap higher than T
 * So gaps that are too big are rejected! (they won't count in the merging)
 * Step3: Defining final blocks.
 * Now need to iterate over each component to collect offset and size.
 * Blocks too big are broken down in the caller itself.
 *
 * Step4: Sorting:
 * Then, need to also sort by power of two. Bigger blocks need to go first and
 * the smaller ones last.
 *
 */
static int* renderv2_schedule_tasks(RenderV2* r, uint32_t* dirty_mask) {
  int nb = r->num_blocks;
  int* off = calloc(nb + 1, sizeof(int));
  int noff = 0;
  int b0, b1;
  for (int ib = 0; ib < nb; ib++) {
    bool dirty = dirty_mask[ib] != 0;
    b0 = r->wire_block[ib].off;
    b1 = r->wire_block[ib].off + r->wire_block[ib].size;
    bool last_clean = (noff % 2) == 0;
    if ((dirty && last_clean) || (!dirty && !last_clean)) {
      noff++;
    }
    off[noff] = b1;
  }
  // noff=0 --> 0 dirty
  if (noff == 0) {
    free(off);
    return NULL;
  }

  // noff=1 --> 1 dirty  0 gap
  // noff=2 --> 1 dirty  0 gap
  // noff=3 --> 2 dirtys 1 gap

  int ngap = (noff - 1) / 2;
  int ng = ngap + 1;
  int* g0 = NULL;
  int* s0 = NULL;
  int* g1 = NULL;
  int* s1 = NULL;
  int num_bin = 10;
  int bins[] = {
      500,   //
      1000,  //
      1500,  //
      2000,  //
      2500,  //
      3000,  //
      3500,  //
      4000,  //
      4500,  //
      5000,  //
  };

  // Total num of tasks scheduled
  int nt = 0;
  for (int i = 0; i < ng; i++) {
    nt += off[2 * i + 1] - off[2 * i];
  }
  for (int i = 0; i < ngap; i++) {
    int i0 = off[2 * i + 1];
    int i1 = off[2 * i + 2];
    arrput(g0, i);
    arrput(s0, i1 - i0);
  }
  arrsetlen(g1, arrlen(g0));
  arrsetlen(s1, arrlen(s0));
  bucket_sort(ngap, g0, s0, num_bin, bins, g1, s1);

  int* rr = calloc(ng, sizeof(int));
  int* cc = calloc(ng, sizeof(int));
  for (int i = 0; i < ng; i++) {
    cc[i] = i;
  }

  int T = 5000;
  int nc = ng;
  for (int ig = 0; ig < ngap; ig++) {
    int s = s1[ig];
    int g = g1[ig];
    bool stop = false;
    if (nt >= T * nc) stop = true;
    if (s > T) stop = true;
    if (stop) break;
    uf_union(cc, rr, g, g + 1);
    nt += s;
    nc -= 1;
  }

  /* post-process components */
  arrsetlen(g0, ng);
  arrsetlen(s0, ng);
  arrsetlen(g1, ng);
  arrsetlen(s1, ng);
  for (int i = 0; i < ng; i++) {
    g0[i] = INT_MAX;
    s0[i] = INT_MIN;
  }
  for (int i = 0; i < ng; i++) {
    int k = uf_find(cc, i);
    int i0 = off[2 * i + 0];
    int i1 = off[2 * i + 1];
    if (g0[k] > i0) g0[k] = i0;
    if (s0[k] < i1) s0[k] = i1;
  }

  // Compress
  int k = 0;
  for (int i = 0; i < ng; i++) {
    if (s0[i] != INT_MIN) {
      s0[k] = s0[i] - g0[i];
      g0[k] = g0[i];
      k++;
    }
  }
  assert(k == nc);

  int bins2[] = {
      1000,   //
      2000,   //
      5000,   //
      8000,   //
      10000,  //
      20000,  //
      40000,  //
  };
  bucket_sort(nc, g0, s0, 7, bins2, g1, s1);

  int* ranges = NULL;
  for (int i = 0; i < nc; i++) {
    int j = nc - i - 1;
    int start = g1[j];
    int end = g1[j] + s1[j];
    arrput(ranges, start);
    arrput(ranges, end);
  }

  arrfree(g0);
  arrfree(s0);
  arrfree(g1);
  arrfree(s1);
  free(cc);
  free(rr);
  free(off);

  if (false) {
    float fnt = nt;
    float fn = arrlen(r->pos);
    float perc = 100 * (fnt / fn);
    printf("(A2) Drawing %f %% .\n", perc);
  }
  return ranges;
}

RenderV2* renderv2_create(int w, int h, int nwire, int nl,
                          RenderTexture2D* layers) {
  RenderV2* r = calloc(1, sizeof(RenderV2));
  r->w = w;
  r->h = h;
  r->nwire = nwire;
  r->pmap = texnew(w, h);
  texclear(r->pmap, BLANK);
  r->full_pmap_update = true;
  r->num_blocks = (r->nwire + 31) / 32;
  r->wire_block = calloc(r->num_blocks, sizeof(WireBlock));
  for (int i = 0; i < r->num_blocks; i++) {
    r->wire_block[i] = (WireBlock){-1, -1};
  }
  r->nl = nl;
  for (int i = 0; i < nl; i++) {
    r->layers[i] = layers[i];
  }
  r->combined_layers = texnew(w, h);
  r->acc_c = texnew(w, h);
  r->acc_l = texnew(w, h);
  texclear(r->acc_c, BLANK);
  texclear(r->acc_l, BLANK);
  r->hide_mask = ~0;
  return r;
}

static inline Vector4 c2fc(Color c) {
  return (Vector4){
      c.r / 255.0f,
      c.g / 255.0f,
      c.b / 255.0f,
      c.a / 255.0f,
  };
}

static void renderv2_render_nand_states_error(RenderV2* r, NandDesc* nidx,
                                              float utime) {
  int w = r->w;
  int nbad = arrlen(r->bad_nand_ids);
  if (nbad == 0) return;

  /* Clear arrays for this frame */
  arrsetlen(r->nandacterr_pos, 0);
  arrsetlen(r->nandacterr_rot, 0);

  for (int i = 0; i < nbad; i++) {
    int id = r->bad_nand_ids[i];
    int idx = nidx[id].idx;
    int rot = nidx[id].rot;

    float xc = idx % w;
    float yc = idx / w;
    xc += 1;
    yc += 1;

    arrput(r->nandacterr_pos, ((Vector2){xc, yc}));
    arrput(r->nandacterr_rot, ((float)rot));
  }

  int ni = arrlen(r->nandacterr_pos);

  /* Update VBOs with bad NAND data */
  rlUpdateVertexBuffer(r->nandacterr_vbo_pos, r->nandacterr_pos,
                       ni * sizeof(Vector2), 0);
  rlUpdateVertexBuffer(r->nandacterr_vbo_rot, r->nandacterr_rot,
                       ni * sizeof(float), 0);

  /* Draw to circuit texture (acc_c) */
  BeginTextureMode(r->acc_c->rt);
  BeginBlendMode(BLEND_ALPHA);

  begin_shader(nandact4err);
  set_shader_int(nandact4err, w, &r->w);
  set_shader_int(nandact4err, h, &r->h);
  set_shader_float(nandact4err, utime, &utime);

  rlEnableVertexArray(r->nandacterr_vao);
  rlDrawVertexArrayInstanced(0, 6, ni);
  rlDisableVertexArray();

  end_shader();
  EndBlendMode();
  EndTextureMode();
}

static void renderv2_render_nand_states_normal(RenderV2* r, int ns,
                                               NandDesc* nidx,
                                               NandState* states, float slack) {
  int w = r->w;

  /* Clear arrays for this frame */
  arrsetlen(r->nandact_pos, 0);
  arrsetlen(r->nandact_cnand1, 0);
  arrsetlen(r->nandact_cnand2, 0);
  arrsetlen(r->nandact_cnand3, 0);
  arrsetlen(r->nandact_phase, 0);

  /* Normal mode: render NANDs based on activation state */
  for (int i = 0; i < ns; i++) {
    int id = states[i].id_nand;
    float act = states[i].activation_counter;
    float max_delay = states[i].max_delay;
    /* These are in "waking up" mode, they're not necessarily awake (ie maybe
     * won't change output value). */
    if (max_delay < 1.0) continue;

    int idx = nidx[id].idx;
    int rot = nidx[id].rot;
    Vector4 c1 = c2fc(nidx[id].c1);
    Vector4 c2 = c2fc(nidx[id].c2);
    Vector4 c3 = c2fc(nidx[id].c3);

    float xc = idx % w;
    float yc = idx / w;
    xc += 1;
    yc += 1;

    float phase_x = (act - slack * 10.0) / max_delay;
    float phase_y = rot;

    arrput(r->nandact_pos, ((Vector2){xc, yc}));
    arrput(r->nandact_cnand1, c1);
    arrput(r->nandact_cnand2, c2);
    arrput(r->nandact_cnand3, c3);
    arrput(r->nandact_phase, ((Vector4){phase_x, phase_y, 0, 0}));
  }

  int ni = arrlen(r->nandact_pos);
  if (ni == 0) {
    return;
  }

  /* Update VBOs with active NAND data */
  rlUpdateVertexBuffer(r->nandact_vbo_pos, r->nandact_pos, ni * sizeof(Vector2),
                       0);
  rlUpdateVertexBuffer(r->nandact_vbo_cnand1, r->nandact_cnand1,
                       ni * sizeof(Vector4), 0);
  rlUpdateVertexBuffer(r->nandact_vbo_cnand2, r->nandact_cnand2,
                       ni * sizeof(Vector4), 0);
  rlUpdateVertexBuffer(r->nandact_vbo_cnand3, r->nandact_cnand3,
                       ni * sizeof(Vector4), 0);
  rlUpdateVertexBuffer(r->nandact_vbo_phase, r->nandact_phase,
                       ni * sizeof(Vector4), 0);

  /* Draw to light texture (acc_l) */
  BeginTextureMode(r->acc_l->rt);
  BeginBlendMode(BLEND_ALPHA);

  begin_shader(nandact4);
  set_shader_int(nandact4, w, &r->w);
  set_shader_int(nandact4, h, &r->h);

  rlEnableVertexArray(r->nandact_vao);
  rlDrawVertexArrayInstanced(0, 6, ni);
  rlDisableVertexArray();

  end_shader();
  EndBlendMode();
  EndTextureMode();
}

static float clamp(float a, float b, float x) {
  if (x < a) return a;
  if (x > b) return b;
  return x;
}

static float smoothstep(float a, float b, float x) {
  float t = clamp(0, 1, (x - a) / (b - a));
  return t * t * (3 - 2 * t);
}

void renderv2_render_err(RenderV2* r, float utime) {
  Cam2D cam = {0};
  cam.sp = 1;
  int n = arrlen(r->err_pos);
  if (n == 0) return;
  BeginTextureMode(r->acc_c->rt);
  BeginBlendMode(BLEND_ALPHA);
  rlPushMatrix();
  rlDisableBackfaceCulling();
  rlDisableDepthMask();
  Shaders* s = get_shaders();
  begin_shader(pixel_error);
  Matrix identity = MatrixIdentity();
  Matrix modelview = rlGetMatrixModelview();
  Matrix projection = rlGetMatrixProjection();
  Matrix mtra = MatrixTranslate(cam.off.x, cam.off.y, 0);
  Matrix msca = MatrixScale(cam.sp, cam.sp, 1);
  modelview = MatrixMultiply(mtra, modelview);
  modelview = MatrixMultiply(msca, modelview);
  Matrix mvp = MatrixMultiply(MatrixMultiply(identity, modelview), projection);

  SetShaderValueMatrix(s->pixel_error_shader, s->pixel_error_loc_mvp, mvp);
  set_shader_float(pixel_error, utime, &utime);
  set_shader_float(pixel_error, zoom, &cam.sp);

  rlEnableVertexArray(r->err_vao);
  rlEnableVertexBuffer(r->err_vbo_vertices);
  rlDrawVertexArrayInstanced(0, 6, n);
  rlDisableVertexArray();
  end_shader();
  rlEnableBackfaceCulling();
  rlEnableDepthMask();
  rlPopMatrix();
  EndBlendMode();
  EndTextureMode();
}

static void renderv2_render_nand(RenderV2* r, int mode) {
  int ni = arrlen(r->nand_pos);
  if (ni == 0) {
    return;
  }

  Shaders* s = get_shaders();
  BeginTextureMode(r->acc_c->rt);

  int loc_pos = s->nand4_aloc_pos;
  int loc_vert = s->nand4_aloc_vert;
  int loc_clr = s->nand4_aloc_clr;
  int off = 0;
  begin_shader(nand4);
  rlEnableVertexArray(r->nand_vao);
  set_shader_int(nand4, w, &r->w);
  set_shader_int(nand4, h, &r->h);
  set_shader_int(nand4, mode, &mode);

  rlEnableVertexBuffer(r->nand_vbo_pos);
  rlSetVertexAttribute(loc_pos, 2, RL_FLOAT, false, 2 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(loc_pos, 1);
  rlEnableVertexAttribute(loc_pos);

  rlEnableVertexBuffer(r->nand_vbo_clr);
  rlSetVertexAttribute(loc_clr, 4, RL_FLOAT, false, 4 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(loc_clr, 1);
  rlEnableVertexAttribute(loc_clr);

  rlDrawLineArrayInstanced(0, 2, ni);
  end_shader();
  EndTextureMode();
}

Tex* renderv2_render(RenderV2* r, Cam2D cam, int tw, int th, int ns,
                     float frame_steps, NandDesc* nidx, NandState* states,
                     bool use_neon, Times times) {
  Tex* tc = texnew(tw, th);
  Tex* tl = texnew(tw, th);
  texclear(tc, r->bg_color);
  texclear(tl, BLACK);
  Tex* light = r->acc_l;
  Tex* circ = r->acc_c;

  float f = 0.9;
  f = 0.8 * smoothstep(16, 128, frame_steps);
  f = 1 - f;

  texmapcircuitlight_v2(r->combined_layers->rt.texture, r->pmap, r->error_mode,
                        times, r->tickmod, r->tickgap, f, &circ, &light);
  renderv2_render_err(r, times.utime);

  if (r->error_mode) {
    renderv2_render_nand_states_error(r, nidx, times.utime);
  } else {
    /* If it's too fast we don't draw nand states (not able to see it anyway) */
    if (frame_steps < 10) {
      renderv2_render_nand_states_normal(r, ns, nidx, states, times.slack);
    }
  }

  texproj(circ, cam, tc);
  texproj(light, cam, tl);

  Tex* combined;
  if (r->error_mode) {
    /* No bloom/gaussian in error mode */
    combined = texbloomcombine(tc, tl);
    texdel(tl);
    texdel(tc);
  } else {
    Tex* smoothed;
    if (use_neon) {
      smoothed = texgauss2(tl);
      texdel(tl);
    } else {
      smoothed = tl;
    }
    combined = texbloomcombine(tc, smoothed);
    texdel(smoothed);
    texdel(tc);
  }
  return combined;
}

static bool is_hidden(RenderV2* r, int l) {
  return (r->hide_mask & (1 << l)) != 0;
}

static void renderv2_combine_layers(RenderV2* r) {
  texclear(r->combined_layers, BLANK);
  if (!is_hidden(r, 0)) {
    texdraw2(r->combined_layers->rt, r->layers[0]);
    // renderv2_render_nand(r, 0);
  }
  for (int i = 1; i < r->nl; i++) {
    if (!is_hidden(r, i)) {
      texdraw2(r->combined_layers->rt, r->layers[i]);
    }
  }
}

void renderv2_update_hidden_mask(RenderV2* r, int hidden_mask) {
  if (hidden_mask != r->hide_mask) {
    r->full_pmap_update = true;
    texclear(r->pmap, BLANK);
    r->hide_mask = hidden_mask;
    renderv2_combine_layers(r);
  }
}

static void renderv2_prepare_nand(RenderV2* r) {
  int n = arrlen(r->nand_clr);
  if (n == 0) return;
  r->nand_vao = rlLoadVertexArray();
  rlEnableVertexArray(r->nand_vao);
  Shaders* s = get_shaders();
  /* Vertices */
  int vert_loc = s->nand4_aloc_vert;
  float vertices[] = {0.0f, 1.0f};
  r->nand_vbo_vert = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
  rlEnableVertexBuffer(r->nand_vbo_vert);
  rlSetVertexAttribute(vert_loc, 1, RL_FLOAT, false, 1 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(vert_loc, 0);  // No divisor - per vertex
  rlEnableVertexAttribute(vert_loc);
  /* Position */
  int pos_loc = s->nand4_aloc_pos;
  r->nand_vbo_pos = rlLoadVertexBuffer(r->nand_pos, n * sizeof(Vector2), false);
  rlEnableVertexBuffer(r->nand_vbo_pos);
  rlSetVertexAttribute(pos_loc, 2, RL_FLOAT, false, sizeof(Vector2), 0);
  rlSetVertexAttributeDivisor(pos_loc, 1);
  rlEnableVertexAttribute(pos_loc);
  /* Colors */
  int clr_loc = s->nand4_aloc_clr;
  r->nand_vbo_clr = rlLoadVertexBuffer(r->nand_clr, n * sizeof(Vector4), false);
  rlEnableVertexBuffer(r->nand_vbo_clr);
  rlSetVertexAttribute(clr_loc, 4, RL_FLOAT, false, sizeof(Vector4), 0);
  rlSetVertexAttributeDivisor(clr_loc, 1);
  rlEnableVertexAttribute(clr_loc);
  rlDisableVertexArray();
}

static void renderv2_prepare_nandact(RenderV2* r) {
  // Number of NANDs (each NAND has 3 color entries in nand_clr)
  int n = arrlen(r->nand_clr) / 3;
  if (n == 0) return;

  r->nandact_vao = rlLoadVertexArray();
  rlEnableVertexArray(r->nandact_vao);
  Shaders* s = get_shaders();

  /* Vertices - static quad (6 vertices for 2 triangles) */
  int vert_loc = s->nandact4_aloc_vert;
  Vector3 vertices[] = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
      {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
  };
  r->nandact_vbo_vert = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
  rlEnableVertexBuffer(r->nandact_vbo_vert);
  rlSetVertexAttribute(vert_loc, 3, RL_FLOAT, false, 3 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(vert_loc, 0);  // Per vertex
  rlEnableVertexAttribute(vert_loc);

  /* Position - dynamic, one per NAND instance */
  int pos_loc = s->nandact4_aloc_pos;
  r->nandact_vbo_pos = rlLoadVertexBuffer(NULL, n * sizeof(Vector2), true);
  rlEnableVertexBuffer(r->nandact_vbo_pos);
  rlSetVertexAttribute(pos_loc, 2, RL_FLOAT, false, sizeof(Vector2), 0);
  rlSetVertexAttributeDivisor(pos_loc, 1);  // Per instance
  rlEnableVertexAttribute(pos_loc);

  /* Color 1 (cnand1) - dynamic */
  int cnand1_loc = s->nandact4_aloc_cnand1;
  r->nandact_vbo_cnand1 = rlLoadVertexBuffer(NULL, n * sizeof(Vector4), true);
  rlEnableVertexBuffer(r->nandact_vbo_cnand1);
  rlSetVertexAttribute(cnand1_loc, 4, RL_FLOAT, false, sizeof(Vector4), 0);
  rlSetVertexAttributeDivisor(cnand1_loc, 1);
  rlEnableVertexAttribute(cnand1_loc);

  /* Color 2 (cnand2) - dynamic */
  int cnand2_loc = s->nandact4_aloc_cnand2;
  r->nandact_vbo_cnand2 = rlLoadVertexBuffer(NULL, n * sizeof(Vector4), true);
  rlEnableVertexBuffer(r->nandact_vbo_cnand2);
  rlSetVertexAttribute(cnand2_loc, 4, RL_FLOAT, false, sizeof(Vector4), 0);
  rlSetVertexAttributeDivisor(cnand2_loc, 1);
  rlEnableVertexAttribute(cnand2_loc);

  /* Color 3 (cnand3) - dynamic */
  int cnand3_loc = s->nandact4_aloc_cnand3;
  r->nandact_vbo_cnand3 = rlLoadVertexBuffer(NULL, n * sizeof(Vector4), true);
  rlEnableVertexBuffer(r->nandact_vbo_cnand3);
  rlSetVertexAttribute(cnand3_loc, 4, RL_FLOAT, false, sizeof(Vector4), 0);
  rlSetVertexAttributeDivisor(cnand3_loc, 1);
  rlEnableVertexAttribute(cnand3_loc);

  /* Phase - dynamic (vec4: x=activation, y=rotation, z,w unused) */
  int phase_loc = s->nandact4_aloc_phase;
  r->nandact_vbo_phase = rlLoadVertexBuffer(NULL, n * sizeof(Vector4), true);
  rlEnableVertexBuffer(r->nandact_vbo_phase);
  rlSetVertexAttribute(phase_loc, 4, RL_FLOAT, false, sizeof(Vector4), 0);
  rlSetVertexAttributeDivisor(phase_loc, 1);
  rlEnableVertexAttribute(phase_loc);

  rlDisableVertexArray();
}

static void renderv2_prepare_nandacterr(RenderV2* r) {
  int n = arrlen(r->bad_nand_ids);
  if (n == 0) return;

  r->nandacterr_vao = rlLoadVertexArray();
  rlEnableVertexArray(r->nandacterr_vao);
  Shaders* s = get_shaders();

  /* Vertices - static quad (6 vertices for 2 triangles) */
  int vert_loc = s->nandact4err_aloc_vert;
  Vector3 vertices[] = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
      {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
  };
  r->nandacterr_vbo_vert =
      rlLoadVertexBuffer(vertices, sizeof(vertices), false);
  rlEnableVertexBuffer(r->nandacterr_vbo_vert);
  rlSetVertexAttribute(vert_loc, 3, RL_FLOAT, false, 3 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(vert_loc, 0);
  rlEnableVertexAttribute(vert_loc);

  /* Position - dynamic */
  int pos_loc = s->nandact4err_aloc_pos;
  r->nandacterr_vbo_pos = rlLoadVertexBuffer(NULL, n * sizeof(Vector2), true);
  rlEnableVertexBuffer(r->nandacterr_vbo_pos);
  rlSetVertexAttribute(pos_loc, 2, RL_FLOAT, false, sizeof(Vector2), 0);
  rlSetVertexAttributeDivisor(pos_loc, 1);
  rlEnableVertexAttribute(pos_loc);

  /* Rotation - dynamic */
  int rot_loc = s->nandact4err_aloc_rot;
  r->nandacterr_vbo_rot = rlLoadVertexBuffer(NULL, n * sizeof(float), true);
  rlEnableVertexBuffer(r->nandacterr_vbo_rot);
  rlSetVertexAttribute(rot_loc, 1, RL_FLOAT, false, sizeof(float), 0);
  rlSetVertexAttributeDivisor(rot_loc, 1);
  rlEnableVertexAttribute(rot_loc);

  rlDisableVertexArray();
}

static void renderv2_prepare_err(RenderV2* r) {
  int n = arrlen(r->err_pos);
  if (n == 0) return;
  Shaders* s = get_shaders();

  r->err_vao = rlLoadVertexArray();
  rlEnableVertexArray(r->err_vao);
  Vector3 vertices[] = {
      {0.0f, 0.0f, 0.0},  {0.0f, 1.0f, 0.0}, {1.0f, 0.0f, 0.0f},
      {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0}, {1.0f, 1.0f, 0.0},
  };
  r->err_vbo_vertices = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
  int vert_loc = s->pixel_error_aloc_vert;
  rlSetVertexAttribute(vert_loc, 3, RL_FLOAT, false, 3 * sizeof(float), 0);
  rlEnableVertexAttribute(vert_loc);

  r->err_vbo_pos = rlLoadVertexBuffer(r->err_pos, n * sizeof(Vector2), false);
  int pos_loc = s->pixel_error_aloc_pos;
  rlEnableVertexBuffer(r->err_vbo_pos);
  rlSetVertexAttribute(pos_loc, 2, RL_FLOAT, false, sizeof(Vector2), 0);
  rlSetVertexAttributeDivisor(pos_loc, 1);
  rlEnableVertexAttribute(pos_loc);

  rlDisableVertexArray();
}

static void renderv2_prepare_pmap(RenderV2* r) {
  r->vao = rlLoadVertexArray();
  rlEnableVertexArray(r->vao);

  // Line vertices: start (0) and end (1)
  float vertices[] = {0.0f, 1.0f};
  int n = arrlen(r->wids);
  r->vbo_wids = rlLoadVertexBuffer(r->wids, n * sizeof(float), false);
  r->vbo_pos = rlLoadVertexBuffer(r->pos, n * sizeof(Vector4), false);
  r->vbo_dist = rlLoadVertexBuffer(r->dist, n * sizeof(Vector2), false);
  Shaders* s = get_shaders();
  int pos_loc = s->wire2_aloc_pos;
  int wid_loc = s->wire2_aloc_wid;
  int vert_loc = s->wire2_aloc_vert;
  int dist_loc = s->wire2_aloc_dist;

  // Line vertices (t parameter: 0.0 = start, 1.0 = end)
  r->vbo_vertices = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
  rlEnableVertexBuffer(r->vbo_vertices);
  rlSetVertexAttribute(vert_loc, 1, RL_FLOAT, false, 1 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(vert_loc, 0);  // No divisor - per vertex
  rlEnableVertexAttribute(vert_loc);

  // ID of the wires
  rlEnableVertexBuffer(r->vbo_wids);
  rlSetVertexAttribute(wid_loc, 1, RL_FLOAT, false, 1 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(wid_loc, 1);
  rlEnableVertexAttribute(wid_loc);

  // Actual position of pixel in image
  rlEnableVertexBuffer(r->vbo_pos);
  rlSetVertexAttribute(pos_loc, 4, RL_FLOAT, false, 4 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(pos_loc, 1);
  rlEnableVertexAttribute(pos_loc);

  // Distances for interpolation
  rlEnableVertexBuffer(r->vbo_dist);
  rlSetVertexAttribute(dist_loc, 2, RL_FLOAT, false, 2 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(dist_loc, 1);
  rlEnableVertexAttribute(dist_loc);

  rlDisableVertexArray();
}

void renderv2_prepare(RenderV2* r, int tickmod, int tickgap) {
  r->tickmod = tickmod;
  r->tickgap = tickgap;
  renderv2_prepare_err(r);
  renderv2_prepare_nand(r);
  renderv2_prepare_nandact(r);
  renderv2_prepare_nandacterr(r);
  renderv2_prepare_pmap(r);
}

static void renderv2_dispatch(RenderV2* r, Shaders* s, int start, int end) {
  if (start == end) {
    return;
  }
  assert(end > start);
  int loc_pos = s->wire2_aloc_pos;
  int loc_wids = s->wire2_aloc_wid;
  int loc_dist = s->wire2_aloc_dist;
  int loc_vert = s->wire2_aloc_vert;
  int nmax = 100000;
  int off = start;
  while (off < end) {
    int ni = (end - off) > nmax ? nmax : (end - off);
    /* WIDs */
    rlEnableVertexBuffer(r->vbo_wids);
    rlSetVertexAttribute(loc_wids, 1, RL_FLOAT, false, 1 * sizeof(float),
                         off * sizeof(int));
    rlSetVertexAttributeDivisor(loc_wids, 1);
    rlEnableVertexAttribute(loc_wids);

    /* Distance */
    rlEnableVertexBuffer(r->vbo_dist);
    rlSetVertexAttribute(loc_dist, 2, RL_FLOAT, false, 2 * sizeof(float),
                         off * sizeof(Vector2));
    rlSetVertexAttributeDivisor(loc_dist, 1);
    rlEnableVertexAttribute(loc_dist);

    /* Positions */
    rlEnableVertexBuffer(r->vbo_pos);
    rlSetVertexAttribute(loc_pos, 4, RL_FLOAT, false, 4 * sizeof(float),
                         off * sizeof(Vector4));
    rlSetVertexAttributeDivisor(loc_pos, 1);
    rlEnableVertexAttribute(loc_pos);

    rlDrawLineArrayInstanced(0, 2, ni);
    off += ni;
  }
}

void renderv2_update_pulse(RenderV2* r, Texture pulses, uint32_t* dirty_mask) {
  BeginTextureMode(r->pmap->rt);
  rlPushMatrix();
  rlEnableDepthTest();
  // rlClearScreenBuffers();  // Clear both color and depth buffers
  rlSetBlendMode(RL_BLEND_CUSTOM);
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  Shaders* s = get_shaders();
  int w = r->w;
  int h = r->h;
  int pulse_w = pulses.width;
  begin_shader(wire2);
  set_shader_int(wire2, w, &w);
  set_shader_int(wire2, h, &h);
  set_shader_int(wire2, pulse_w, &pulse_w);
  set_shader_int(wire2, hide_mask, &r->hide_mask);

  int tickmod64 = r->tickmod * 64;
  set_shader_int(wire2, tickmod64, &tickmod64);
  Matrix identity = MatrixIdentity();
  Matrix modelview = rlGetMatrixModelview();
  Matrix projection = rlGetMatrixProjection();
  Matrix mvp = MatrixMultiply(MatrixMultiply(identity, modelview), projection);
  mvp = MatrixIdentity();
  SetShaderValueMatrix(s->wire2_shader, s->wire2_loc_mvp, mvp);
  rlEnableVertexArray(r->vao);
  rlEnableShader(get_shaders()->wire2_shader.id);
  rlActiveTextureSlot(0);
  rlEnableTexture(pulses.id);
  set_shader_int(wire2, error_mode, &r->error_mode);
  int nmax = 10000;
  int n = arrlen(r->pos);
  int algo = 2;
  /* On first frame, forces drawing everything.
   * Only relevant for error mode, since the dirty flags are not
   * updated. */
  if (r->full_pmap_update) {
    algo = 0;
    r->full_pmap_update = false;
  }

  if (algo == 0) {
    renderv2_dispatch(r, s, 0, n);
  }
  if (algo == 1) {
    int T = 20000;
    int skip = 4000;
    // Taking into account dirty masks
    int nb = r->num_blocks;
    int start = -1;
    int actual = 0;
    int end = -1;
    for (int i = 0; i < nb; i++) {
      bool dirty = dirty_mask[i] != 0;
      // dirty = true;
      int b0 = r->wire_block[i].off;
      int b1 = r->wire_block[i].off + r->wire_block[i].size;
      if (dirty) {
        if (start == -1) {
          start = b0;
          end = b1;
        } else {
          end = b1;
        }
      } else {
        if (end != -1) {
          if (b1 - end > skip) {
            int nt = end - start;
            actual += nt;
            renderv2_dispatch(r, s, start, end);
            start = -1;
            end = -1;
          }
        }
      }
      // When do I dispatch?
    }
    if (start != -1) {
      int nt = end - start;
      actual += nt;
      renderv2_dispatch(r, s, start, end);
    }
    if (false) {
      float fnt = actual;
      float fn = n;
      float perc = 100 * (fnt / fn);
      printf("Drawing %f %% .\n", perc);
    }
  }
  if (algo == 2) {
    int* ranges = renderv2_schedule_tasks(r, dirty_mask);
    int nr = arrlen(ranges) / 2;
    for (int i = 0; i < nr; i++) {
      int start = ranges[2 * i];
      int end = ranges[2 * i + 1];
      renderv2_dispatch(r, s, start, end);
    }
    arrfree(ranges);
  }
  rlDisableVertexArray();
  EndShaderMode();
  rlDisableDepthTest();
  rlPopMatrix();
  rlSetBlendMode(RL_BLEND_ALPHA);
  EndTextureMode();
}

void renderv2_free(RenderV2* r) {
  free(r->wire_block);
  texdel(r->pmap);
  texdel(r->acc_l);
  texdel(r->acc_c);
  texdel(r->combined_layers);
  arrfree(r->pos);
  arrfree(r->wids);
  arrfree(r->dist);
  if (r->err_pos) {
    rlUnloadVertexBuffer(r->err_vbo_pos);
    rlUnloadVertexBuffer(r->err_vbo_vertices);
    rlUnloadVertexArray(r->err_vao);
  }
  arrfree(r->err_pos);
  rlUnloadVertexArray(r->vao);
  rlUnloadVertexBuffer(r->vbo_pos);
  rlUnloadVertexBuffer(r->vbo_wids);
  rlUnloadVertexBuffer(r->vbo_dist);
  rlUnloadVertexBuffer(r->vbo_vertices);

  arrfree(r->nand_pos);
  arrfree(r->nand_clr);
  rlUnloadVertexArray(r->nand_vao);
  rlUnloadVertexBuffer(r->nand_vbo_pos);
  rlUnloadVertexBuffer(r->nand_vbo_clr);
  rlUnloadVertexBuffer(r->nand_vbo_vert);

  /* Free nandact resources */
  arrfree(r->nandact_pos);
  arrfree(r->nandact_cnand1);
  arrfree(r->nandact_cnand2);
  arrfree(r->nandact_cnand3);
  arrfree(r->nandact_phase);
  arrfree(r->bad_nand_ids);
  if (r->nandact_vao) {
    rlUnloadVertexArray(r->nandact_vao);
    rlUnloadVertexBuffer(r->nandact_vbo_vert);
    rlUnloadVertexBuffer(r->nandact_vbo_pos);
    rlUnloadVertexBuffer(r->nandact_vbo_cnand1);
    rlUnloadVertexBuffer(r->nandact_vbo_cnand2);
    rlUnloadVertexBuffer(r->nandact_vbo_cnand3);
    rlUnloadVertexBuffer(r->nandact_vbo_phase);
  }

  /* Free nandacterr resources */
  arrfree(r->nandacterr_pos);
  arrfree(r->nandacterr_rot);
  if (r->nandacterr_vao) {
    rlUnloadVertexArray(r->nandacterr_vao);
    rlUnloadVertexBuffer(r->nandacterr_vbo_vert);
    rlUnloadVertexBuffer(r->nandacterr_vbo_pos);
    rlUnloadVertexBuffer(r->nandacterr_vbo_rot);
  }

  free(r);
}

void renderv2_add_err_pixel(RenderV2* r, int x, int y) {
  Vector2 v = {x, y};
  arrput(r->err_pos, v);
}

void renderv2_add_bad_nand(RenderV2* r, int nand_id) {
  arrput(r->bad_nand_ids, nand_id);
}

static inline Vector4 clr2vec4(Color c) {
  return (Vector4){
      c.r / 255.0f,
      c.g / 255.0f,
      c.b / 255.0f,
      c.a / 255.0f,
  };
}

void renderv2_addnand(RenderV2* r, Vector2 p0, Vector2 p1, Vector2 p2, Color c0,
                      Color c1, Color c2) {
  arrput(r->nand_pos, p0);
  arrput(r->nand_pos, p1);
  arrput(r->nand_pos, p2);
  arrput(r->nand_clr, clr2vec4(c0));
  arrput(r->nand_clr, clr2vec4(c1));
  arrput(r->nand_clr, clr2vec4(c2));
}
