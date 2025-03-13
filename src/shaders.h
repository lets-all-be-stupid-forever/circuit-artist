#ifndef SHADERS_H
#define SHADERS_H
#include "raylib.h"

#define set_shader_vec2(name, loc, val)                                       \
  SetShaderValue(GetShaders()->name##_shader, GetShaders()->name##_loc_##loc, \
                 (val), SHADER_UNIFORM_VEC2);

#define set_shader_vec4(name, loc, val)                                       \
  SetShaderValue(GetShaders()->name##_shader, GetShaders()->name##_loc_##loc, \
                 (val), SHADER_UNIFORM_VEC4);

#define set_shader_ivec2(name, loc, val)                                      \
  SetShaderValue(GetShaders()->name##_shader, GetShaders()->name##_loc_##loc, \
                 (val), SHADER_UNIFORM_IVEC2);

#define set_shader_tex(name, loc, val)               \
  SetShaderValueTexture(GetShaders()->name##_shader, \
                        GetShaders()->name##_loc_##loc, (val));

#define set_shader_int(name, loc, val)                                        \
  SetShaderValue(GetShaders()->name##_shader, GetShaders()->name##_loc_##loc, \
                 (val), SHADER_UNIFORM_INT);

#define begin_shader(name) BeginShaderMode(GetShaders()->name##_shader)
#define end_shader() EndShaderMode()

typedef struct {
  Shader comb_shader;
  int comb_loc_dst_tex;
  int comb_loc_off;
  int comb_loc_src_size;
  int comb_loc_dst_size;
  int comb_loc_roi_size;
  int comb_loc_src_off;
  int comb_loc_dst_off;

  Shader fill_shader;

  Shader rotate_shader;
  int rotate_loc_ccw;

  Shader project_shader;
  int project_loc_sp;
  int project_loc_img_size;
  int project_loc_tpl;

  Shader update_shader;
  int update_loc_img_size;
  int update_loc_sel_size;
  int update_loc_sel_off;
  int update_loc_sel;
  int update_loc_tool_size;
  int update_loc_tool_off;
  int update_loc_tool;
  int update_loc_mode;
  int update_loc_simu_state;
  int update_loc_bg_color;
  int update_loc_bugged_color;
  int update_loc_undefined_color;
  int update_loc_comp_x;
  int update_loc_comp_y;
  int update_loc_state_buf;

} Shaders;

void InitShaders();
Shaders* GetShaders();

#endif
