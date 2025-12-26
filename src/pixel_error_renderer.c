#include "pixel_error_renderer.h"

#include "raymath.h"
#include "rlgl.h"
#include "shaders.h"
#include "stb_ds.h"

void pixel_error_renderer_init(pixel_error_renderer_t* r) {
  *r = (pixel_error_renderer_t){0};
}

void pixel_error_renderer_add_pixel(pixel_error_renderer_t* r, int x, int y) {
  arrput(r->pos, x);
  arrput(r->pos, y);
}

void pixel_error_renderer_prepare(pixel_error_renderer_t* r) {
  int n = arrlen(r->pos) / 2;
  Vector2* pos = malloc(n * sizeof(Vector2));
  for (int i = 0; i < n; i++) {
    pos[i] = (Vector2){r->pos[2 * i + 0], r->pos[2 * i + 1]};
  }
  r->vbo = rlLoadVertexBuffer(pos, n * sizeof(Vector2), false);
  free(pos);
  Shaders* s = get_shaders();
  int vert_loc = s->pixel_error_aloc_vert;
  int pos_loc = s->pixel_error_aloc_pos;

  r->vao = rlLoadVertexArray();
  rlEnableVertexArray(r->vao);
  Vector3 vertices[] = {
      {0.0f, 0.0f, 0.0},  {0.0f, 1.0f, 0.0}, {1.0f, 0.0f, 0.0f},
      {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0}, {1.0f, 1.0f, 0.0},
  };
  r->vbo_vertices = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
  rlSetVertexAttribute(vert_loc, 3, RL_FLOAT, false, 3 * sizeof(float), 0);
  rlEnableVertexAttribute(vert_loc);

  rlEnableVertexBuffer(r->vbo);
  rlSetVertexAttribute(pos_loc, 2, RL_FLOAT, false, sizeof(Vector2), 0);
  rlSetVertexAttributeDivisor(pos_loc, 1);
  rlEnableVertexAttribute(pos_loc);

  rlDisableVertexArray();
}

void pixel_error_renderer_render(pixel_error_renderer_t* r, Cam2D cam,
                                 float utime) {
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

  int n = arrlen(r->pos) / 2;
  rlEnableVertexArray(r->vao);
  rlEnableVertexBuffer(r->vbo);
  rlDrawVertexArrayInstanced(0, 6, n);
  rlDisableVertexArray();
  end_shader();
  rlEnableBackfaceCulling();
  rlEnableDepthMask();
  rlPopMatrix();
  EndBlendMode();
}

void pixel_error_renderer_destroy(pixel_error_renderer_t* r) {
  arrfree(r->pos);
  rlUnloadVertexArray(r->vao);
  rlUnloadVertexBuffer(r->vbo);
  rlUnloadVertexBuffer(r->vbo_vertices);
}

