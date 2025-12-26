#include "wire_rendering.h"

#include "assert.h"
#include "profiler.h"
#include "raymath.h"
#include "rlgl.h"
#include "shaders.h"
#include "sim.h"
#include "stb_ds.h"
#include "stdio.h"

void wire_renderer_init(WireRenderer* v, int w, int h) {
  v->w = w;
  v->h = h;
  v->error_mode = 0;
  v->tgt = LoadRenderTexture(w, h);
  v->tgt_bright = LoadRenderTexture(w, h);
  // v->pmap = LoadRenderTexture(w, h);
  v->pmap = texnew(w, h);
  // BeginTextureMode(v->pmap);
  // ClearBackground(BLUE);
  // EndTextureMode();
}

void wire_renderer_set_dmap(WireRenderer* v, float* dmap) {
  profiler_tic_single("dmap_copy");
  Image tmp = GenImageColor(v->w, v->h, BLANK);
  memcpy(tmp.data, dmap, v->w * v->h * sizeof(float));
  v->dmap = LoadTextureFromImage(tmp);
  UnloadImage(tmp);
  profiler_tac_single("dmap_copy");
}

void wire_renderer_prepare(WireRenderer* v, int num_wires, int num_segs,
                           WireSegment* seglist) {
  int n = num_segs;
  v->num_wires = num_wires;
  v->numinst = n;
  int* wids = malloc(n * sizeof(int));
  Vector4* pos = malloc(n * sizeof(Vector4));
  int w = v->w;
  int p = 0;
  for (int i = 0; i < n; i++) {
    int wid = seglist[i].wire_id;
    int idx0 = seglist[i].idx_start;
    int idx1 = seglist[i].idx_end;
    int ix0 = idx0 % w;
    int iy0 = idx0 / w;
    int ix1 = idx1 % w;
    int iy1 = idx1 / w;
    // Positive wid correspond to horizontal segments,
    // and negative wids to vertical segments

    /* vertical segment */
    if (iy0 != iy1) {
      wid = -wid - 1;
    }
    wids[p] = wid;
    float eps = 0.0;
    Color color = WHITE;
    if (ix0 == ix1) {
      int y0 = iy0;
      int y1 = iy1;
      if (y0 > y1) {
        int tmp = y1;
        y1 = y0;
        y0 = tmp;
      }
      pos[p++] = (Vector4){ix0, y0, ix0 + 1, y1 + 1};
      assert(ix0 == ix1);
    } else {
      int x0 = ix0;
      int x1 = ix1;
      if (x0 > x1) {
        int tmp = x1;
        x1 = x0;
        x0 = tmp;
      }
      assert(iy0 == iy1);
      pos[p++] = (Vector4){x0, iy0, x1 + 1, iy1 + 1};
    }
  }
  int tot = v->num_wires;

  v->vao = rlLoadVertexArray();
  rlEnableVertexArray(v->vao);
  Shaders* s = get_shaders();
  Vector3 vertices[] = {
      {0.0f, 0.0f, 0.0},  {0.0f, 1.0f, 0.0}, {1.0f, 0.0f, 0.0f},
      {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0}, {1.0f, 1.0f, 0.0},
  };
  v->vbo_wids = rlLoadVertexBuffer(wids, n * sizeof(float), false);
  v->vbo_pos = rlLoadVertexBuffer(pos, n * sizeof(Vector4), false);
  int pos_loc = s->wire_aloc_pos;
  int wid_loc = s->wire_aloc_wid;
  int vert_loc = s->wire_aloc_vert;

  // Rectangle vertices
  v->vbo_vertices = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
  rlSetVertexAttribute(vert_loc, 3, RL_FLOAT, false, 3 * sizeof(float), 0);
  rlEnableVertexAttribute(vert_loc);

  // ID of the wires
  rlEnableVertexBuffer(v->vbo_wids);
  rlSetVertexAttribute(wid_loc, 1, RL_FLOAT, false, 1 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(wid_loc, 1);
  rlEnableVertexAttribute(wid_loc);

  // Actual position of pixel in image
  rlEnableVertexBuffer(v->vbo_pos);
  rlSetVertexAttribute(pos_loc, 4, RL_FLOAT, false, 4 * sizeof(float), 0);
  rlSetVertexAttributeDivisor(pos_loc, 1);
  rlEnableVertexAttribute(pos_loc);
  rlDisableVertexArray();
  free(pos);
  free(wids);
}

void wire_renderer_update_pmap(WireRenderer* v, Texture pulses) {
  BeginTextureMode(v->pmap->rt);
  rlPushMatrix();
  rlDisableBackfaceCulling();
  rlDisableDepthMask();
  rlSetBlendMode(RL_BLEND_CUSTOM);
  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
  Shaders* s = get_shaders();
  int w = v->w;
  int h = v->h;
  int pulse_w = pulses.width;
  begin_shader(wire);
  set_shader_int(wire, w, &w);
  set_shader_int(wire, h, &h);
  set_shader_int(wire, pulse_w, &pulse_w);

  SetTextureFilter(v->dmap, TEXTURE_FILTER_POINT);

  Matrix identity = MatrixIdentity();
  Matrix modelview = rlGetMatrixModelview();
  Matrix projection = rlGetMatrixProjection();
  Matrix mvp = MatrixMultiply(MatrixMultiply(identity, modelview), projection);
  mvp = MatrixIdentity();
  SetShaderValueMatrix(s->wire_shader, s->wire_loc_mvp, mvp);
  rlEnableVertexArray(v->vao);
  rlEnableShader(get_shaders()->wire_shader.id);

  rlActiveTextureSlot(0);
  rlEnableTexture(pulses.id);

  rlActiveTextureSlot(1);
  rlEnableTexture(v->dmap.id);
  int dmap_slot = 1;
  set_shader_int(wire, dmap, &dmap_slot);
  // set_shader_tex(wire, dmap, v->dmap);

  set_shader_int(wire, error_mode, &v->error_mode);

  int nmax = 10000;
  int n = v->numinst;
  int off = 0;
  int loc_pos = s->wire_aloc_pos;
  int loc_wids = s->wire_aloc_wid;
  while (off < n) {
    int ni = (n - off) > nmax ? nmax : (n - off);
    rlEnableVertexBuffer(v->vbo_wids);
    rlSetVertexAttribute(loc_wids, 1, RL_FLOAT, false, 1 * sizeof(float),
                         off * sizeof(int));
    rlSetVertexAttributeDivisor(loc_wids, 1);
    rlEnableVertexAttribute(loc_wids);

    rlEnableVertexBuffer(v->vbo_pos);
    rlSetVertexAttribute(loc_pos, 4, RL_FLOAT, false, 4 * sizeof(float),
                         off * sizeof(Vector4));
    rlSetVertexAttributeDivisor(loc_pos, 1);
    rlEnableVertexAttribute(loc_pos);

    rlDrawVertexArrayInstanced(0, 6, ni);
    off += ni;
  }
  rlDisableVertexArray();

  EndShaderMode();
  rlEnableBackfaceCulling();
  rlEnableDepthMask();
  rlPopMatrix();
  rlSetBlendMode(RL_BLEND_ALPHA);
  EndTextureMode();
}

void wire_renderer_destroy(WireRenderer* v) {
  UnloadRenderTexture(v->tgt);
  UnloadRenderTexture(v->tgt_bright);
  texdel(v->pmap);
  UnloadTexture(v->dmap);
  rlUnloadVertexArray(v->vao);
  rlUnloadVertexBuffer(v->vbo_pos);
  rlUnloadVertexBuffer(v->vbo_wids);
  rlUnloadVertexBuffer(v->vbo_vertices);
  v->tgt = (RenderTexture2D){0};
}

void wire_renderer_set_error_mode(WireRenderer* v, int error_mode) {
  v->error_mode = error_mode;
}
