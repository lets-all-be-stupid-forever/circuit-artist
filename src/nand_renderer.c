#include "nand_renderer.h"

#include "assert.h"
#include "raymath.h"
#include "rlgl.h"
#include "shaders.h"
#include "stb_ds.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"

#define GL_INT 0x1404

void nand_renderer_init(NandRenderer* r) {
  *r = (NandRenderer){0};
  r->error_mode = 0;
  r->nand_tex = ui_get_sprites();
  float tw = r->nand_tex.width;
  float th = r->nand_tex.height;
  int offy = 368;  // 352
  r->roi[0] = (Vector4){32 / tw, offy / th, 6 / tw, 5 / th};
  // r->roi[0] = (Vector4){240 / tw, 320 / th, 192 / tw, 160 / th};
  // r->roi[0] = (Vector4){816 / tw, 272 / th, 192 / tw, 160 / th};
  r->roi[1] = (Vector4){0 / tw, offy / th, 5 / tw, 6 / th};
  r->roi[2] = (Vector4){48 / tw, offy / th, 6 / tw, 5 / th};
  r->roi[3] = (Vector4){16 / tw, offy / th, 5 / tw, 6 / th};
}

void nand_renderer_add(NandRenderer* r, int rot, int x, int y, Color c1,
                       Color c2, Color c3, int bugged) {
  arrput(r->pos[rot], x);
  arrput(r->pos[rot], y);
  arrput(r->colors[rot], c1);
  arrput(r->colors[rot], c2);
  arrput(r->colors[rot], c3);
  arrput(r->bugged[rot], bugged);
}

static inline void find_nand_delta(int rot, int* dx0, int* dx1, int* dy0,
                                   int* dy1) {
  switch (rot) {
    // Right
    // dx = (0, 6)
    // dy = (-1, 4)
    //    - - -
    //  R - N - -
    //    - - N - W
    //  W - N - -
    //    - - -
    case 0: {
      *dx0 = 0;
      *dx1 = 6;
      *dy0 = -1;
      *dy1 = 4;
      break;
    }
      // Down
      // Reference is at (0,0)
      // So, the spam should be:
      // (-1, 4)
      // (0, 6)
      //
      // (x: 0->5)
      // (y: 0->6)
      //
      //  - R   W
      //  - - - - -
      //  - N - N -
      //  - - N - -
      //    - - -
      //      W
    case 1: {
      *dx0 = -1;
      *dx1 = 4;
      *dy0 = 0;
      *dy1 = 6;
      break;
    }
    // Left
    // dx = (0, 6)
    // dy = (-1, 4)
    // * * - - - *
    // R - - N - W
    // W - N - - *
    // * - - N - W
    // * * - - - *
    case 2: {
      *dx0 = 0;
      *dx1 = 6;
      *dy0 = -1;
      *dy1 = 4;
      break;
    }
      // Up
      // Reference is at (0,0)
      // So, the spam should be:
      // (-1, 4)
      // (+3, 6)
      //
      // (x: 0->5)
      // (y: 0->6)
      //
      //    R W
      //    - - -
      //  - - N - -
      //  - N - N -
      //  - - - - -
      //    W   W
    case 3: {
      *dx0 = -1;
      *dx1 = 4;
      *dy0 = 0;
      *dy1 = 6;
      break;
    }
  }
}

void nand_renderer_prepare(NandRenderer* r) {
  for (int rot = 0; rot < 4; rot++) {
    int n = arrlen(r->pos[rot]) / 2;
    int ww = -1;
    int hh = -1;
    int dx0, dx1;
    int dy0, dy1;
    find_nand_delta(rot, &dx0, &dx1, &dy0, &dy1);
    int* rpos = r->pos[rot];
    int* rbugged = r->bugged[rot];
    Color* rcolor = r->colors[rot];
    typedef struct {
      Vector4 pos;
      Color c1;
      Color c2;
      Color c3;
      int bugged;
    } s_nand;

    s_nand* gpu_s = malloc(n * sizeof(s_nand));
    for (int i = 0; i < n; i++) {
      int xx = rpos[2 * i + 0];
      int yy = rpos[2 * i + 1];
      gpu_s[i].bugged = rbugged[i];
      gpu_s[i].c1 = rcolor[3 * i + 0];
      gpu_s[i].c2 = rcolor[3 * i + 1];
      gpu_s[i].c3 = rcolor[3 * i + 2];
      gpu_s[i].pos = (Vector4){
          xx + dx0,
          yy + dy0,
          xx + dx1,
          yy + dy1,
      };
    }
    r->vbo[rot] = rlLoadVertexBuffer(gpu_s, n * sizeof(s_nand), false);
    free(gpu_s);

    r->vao[rot] = rlLoadVertexArray();
    Shaders* s = get_shaders();
    Vector3 vertices[] = {
        {0.0f, 0.0f, 0.0},  {0.0f, 1.0f, 0.0}, {1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0}, {1.0f, 1.0f, 0.0},
    };

    rlEnableVertexArray(r->vao[rot]);

    // Rectangle vertices
    r->vbo_vertices[rot] =
        rlLoadVertexBuffer(vertices, sizeof(vertices), false);
    int vert_loc = s->nand_aloc_vert;
    rlSetVertexAttribute(vert_loc, 3, RL_FLOAT, false, 3 * sizeof(float), 0);
    rlEnableVertexAttribute(vert_loc);

    rlEnableVertexBuffer(r->vbo[rot]);
    int pos_loc = s->nand_aloc_pos;
    int c1_loc = s->nand_aloc_c1;
    int c2_loc = s->nand_aloc_c2;
    int c3_loc = s->nand_aloc_c3;
    int bugged_loc = s->nand_aloc_bugged;

    rlSetVertexAttribute(pos_loc, 4, RL_FLOAT, false, sizeof(s_nand),
                         offsetof(s_nand, pos));
    rlSetVertexAttributeDivisor(pos_loc, 1);
    rlEnableVertexAttribute(pos_loc);

    rlSetVertexAttribute(c1_loc, 4, RL_UNSIGNED_BYTE, true, sizeof(s_nand),
                         offsetof(s_nand, c1));
    rlSetVertexAttributeDivisor(c1_loc, 1);
    rlEnableVertexAttribute(c1_loc);

    rlSetVertexAttribute(c2_loc, 4, RL_UNSIGNED_BYTE, true, sizeof(s_nand),
                         offsetof(s_nand, c2));
    rlSetVertexAttributeDivisor(c2_loc, 1);
    rlEnableVertexAttribute(c2_loc);

    rlSetVertexAttribute(c3_loc, 4, RL_UNSIGNED_BYTE, true, sizeof(s_nand),
                         offsetof(s_nand, c3));
    rlSetVertexAttributeDivisor(c3_loc, 1);
    rlEnableVertexAttribute(c3_loc);

    rlSetVertexAttribute(bugged_loc, 1, GL_INT, false, sizeof(s_nand),
                         offsetof(s_nand, bugged));
    rlSetVertexAttributeDivisor(bugged_loc, 1);
    rlEnableVertexAttribute(bugged_loc);

    rlDisableVertexArray();
  }
}

void nand_renderer_render(NandRenderer* r, Cam2D cam, float utime) {
  BeginBlendMode(BLEND_ALPHA);
  rlPushMatrix();
  rlDisableBackfaceCulling();
  rlDisableDepthMask();
  Shaders* s = get_shaders();
  begin_shader(nand);
  SetTextureFilter(r->nand_tex, TEXTURE_FILTER_POINT);
  SetTextureWrap(r->nand_tex, TEXTURE_WRAP_CLAMP);
  Matrix identity = MatrixIdentity();
  Matrix modelview = rlGetMatrixModelview();
  Matrix projection = rlGetMatrixProjection();
  Matrix mtra = MatrixTranslate(cam.off.x, cam.off.y, 0);
  Matrix msca = MatrixScale(cam.sp, cam.sp, 1);
  modelview = MatrixMultiply(mtra, modelview);
  modelview = MatrixMultiply(msca, modelview);
  assert(r->nand_tex.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  Matrix mvp = MatrixMultiply(MatrixMultiply(identity, modelview), projection);
  SetShaderValueMatrix(s->nand_shader, s->nand_loc_mvp, mvp);
  int loc_pos = s->nand_aloc_pos;
  for (int rot = 0; rot < 4; rot++) {
    set_shader_vec4(nand, roi, &r->roi[rot]);
    set_shader_int(nand, error_mode, &r->error_mode);
    set_shader_float(nand, utime, &utime);
    int off = 0;
    int n = arrlen(r->pos[rot]) / 2;
    int ni = n;
    int tu = 1;
    rlActiveTextureSlot(tu);
    rlEnableTexture(r->nand_tex.id);  // Your integer texture ID
    set_shader_int(nand, ref, &tu);
    rlEnableVertexArray(r->vao[rot]);
    rlEnableVertexBuffer(r->vbo[rot]);
    rlDrawVertexArrayInstanced(0, 6, ni);
    rlDisableVertexArray();
    rlDisableTexture();
  }
  end_shader();
  rlEnableBackfaceCulling();
  rlEnableDepthMask();
  rlPopMatrix();
  EndBlendMode();
}

void nand_renderer_destroy(NandRenderer* r) {
  for (int i = 0; i < 4; i++) {
    arrfree(r->pos[i]);
    arrfree(r->colors[i]);
    arrfree(r->bugged[i]);
    r->pos[i] = NULL;
    r->colors[i] = NULL;
    r->bugged[i] = NULL;
    rlUnloadVertexBuffer(r->vbo[i]);
    rlUnloadVertexBuffer(r->vbo_vertices[i]);
    rlUnloadVertexArray(r->vao[i]);
  }
}

void nand_renderer_set_error_mode(NandRenderer* r, int error_mode) {
  r->error_mode = 1;
}

static inline Vector4 c2fc(Color c) {
  return (Vector4){
      c.r / 255.0f,
      c.g / 255.0f,
      c.b / 255.0f,
      c.a / 255.0f,
  };
}

void render_nand_states(int w, NandDesc* nidx, int ns, NandState* states,
                        Cam2D cam, float slack, float utime) {
  // Rectangle source = {
  //     448,
  //     192,
  //     80,
  //     80,
  // };
  Texture tex = ui_get_sprites();
  Rectangle source = {
      0,
      0,
      tex.width,
      tex.height,
  };

  // TODO: Use mesh instancing
  shader_load("nandact2");
  for (int i = 0; i < ns; i++) {
    int id = states[i].id_nand;
    // max counter is 100
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

    float r = 5.0;
    Rectangle dest = (Rectangle){
        cam.off.x + cam.sp * (xc - r),
        cam.off.y + cam.sp * (yc - r),  // <-- Wrong!
        cam.sp * (2 * r - 1),
        cam.sp * (2 * r - 1),
    };
    Color c = WHITE;
    c.a = 100;
    Vector2 phase = {(act - slack * 10.0) / max_delay, rot};
    // printf("fx=%f act/delay=%f act=%f slack=%f\n", phase.x, act / max_delay,
    //        act, slack);
    // Vector2 vslack = {slack, slack};
    rlDrawRenderBatchActive();
    shader_vec2("phase", &phase);
    shader_vec4("cnand1", &c1);
    shader_vec4("cnand2", &c2);
    shader_vec4("cnand3", &c3);
    // shader_vec2("slack", &vslack);
    DrawTexturePro(tex, source, dest, (Vector2){0, 0}, 0, c);
  }
  shader_unload();
}
