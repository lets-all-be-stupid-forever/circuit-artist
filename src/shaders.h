#ifndef CA_SHADERS_H
#define CA_SHADERS_H
#include "common.h"

#define set_shader_vec2(name, loc, val)        \
  SetShaderValue(get_shaders()->name##_shader, \
                 get_shaders()->name##_loc_##loc, (val), SHADER_UNIFORM_VEC2);

#define set_shader_vec4(name, loc, val)        \
  SetShaderValue(get_shaders()->name##_shader, \
                 get_shaders()->name##_loc_##loc, (val), SHADER_UNIFORM_VEC4);

#define set_shader_ivec2(name, loc, val)                 \
  SetShaderValue(get_shaders()->name##_shader,           \
                 get_shaders()->name##_loc_##loc, (val), \
                 SHADER_UNIFORM_IVEC2);

#define set_shader_tex(name, loc, val)                \
  SetShaderValueTexture(get_shaders()->name##_shader, \
                        get_shaders()->name##_loc_##loc, (val));

#define set_shader_int(name, loc, val)         \
  SetShaderValue(get_shaders()->name##_shader, \
                 get_shaders()->name##_loc_##loc, (val), SHADER_UNIFORM_INT);

#define set_shader_float(name, loc, val)                 \
  SetShaderValue(get_shaders()->name##_shader,           \
                 get_shaders()->name##_loc_##loc, (val), \
                 SHADER_UNIFORM_FLOAT);

#define begin_shader(name) BeginShaderMode(get_shaders()->name##_shader)
#define end_shader() EndShaderMode()

typedef struct {
  Shader comb_shader;
  int comb_loc_dst_tex;
  // int comb_loc_off;
  int comb_loc_src_size;
  int comb_loc_dst_size;
  // int comb_loc_roi_size;
  int comb_loc_src_off;
  int comb_loc_dst_off;
  Shader fill_shader;
  Shader rotate_shader;
  int rotate_loc_ccw;

  Shader project_shader;
  int project_loc_sp;
  int project_loc_img_size;

  Shader project_pattern_shader;
  int project_pattern_loc_sp;
  int project_pattern_loc_pattern;
  int project_pattern_loc_img_size;
  int project_pattern_loc_pad;

  Shader thumbnail_shader;
  int thumbnail_loc_sp;
  int thumbnail_loc_img_size;
  Shader update_shader;
  int update_loc_img_size;
  int update_loc_sel_size;
  int update_loc_sel_off;
  int update_loc_sel;
  int update_loc_tool_size;
  int update_loc_tool_off;
  int update_loc_tool;
  Shader selrect_shader;
  int selrect_loc_rsize;
  int selrect_loc_rect_pos;
  int selrect_loc_pattern_shift;
  int selrect_loc_pattern_width;

  Shader wire_shader;
  int wire_aloc_vert;
  int wire_aloc_pos;
  int wire_aloc_wid;
  int wire_loc_w;
  int wire_loc_slack;
  int wire_loc_pulses;
  int wire_loc_h;
  int wire_loc_mvp;
  int wire_loc_dmap;
  int wire_loc_pulse_w;
  int wire_loc_cur_tick;
  int wire_loc_utime;
  int wire_loc_ref;
  int wire_loc_error_mode;

  Shader wire2_shader;
  int wire2_aloc_vert;
  int wire2_aloc_pos;
  int wire2_aloc_wid;
  int wire2_aloc_dist;
  int wire2_loc_hide_mask;
  int wire2_loc_w;
  int wire2_loc_slack;
  int wire2_loc_pulses;
  int wire2_loc_h;
  int wire2_loc_mvp;
  // int wire2_loc_dmap;
  int wire2_loc_pulse_w;
  int wire2_loc_cur_tick;
  int wire2_loc_utime;
  int wire2_loc_ref;
  int wire2_loc_error_mode;
  int wire2_loc_tickmod64;

  Shader nand_shader;
  int nand_aloc_vert;
  int nand_aloc_pos;
  int nand_aloc_c1;
  int nand_aloc_c2;
  int nand_aloc_c3;
  int nand_aloc_bugged;
  int nand_loc_roi;
  int nand_loc_ref;
  int nand_loc_utime;
  int nand_loc_error_mode;
  int nand_loc_mvp;

  Shader pixel_error_shader;
  int pixel_error_aloc_pos;
  int pixel_error_aloc_vert;
  int pixel_error_loc_utime;
  int pixel_error_loc_mvp;
  int pixel_error_loc_zoom;

  Shader wire_combine_shader;
  int wire_combine_loc_pmap;
  int wire_combine_loc_segsize;
  int wire_combine_loc_dmap;
  int wire_combine_loc_tick;
  int wire_combine_loc_slack;
  int wire_combine_loc_error_mode;
  int wire_combine_loc_utime;

  Shader wire_combine2_shader;
  int wire_combine2_loc_pmap;
  int wire_combine2_loc_segsize;
  int wire_combine2_loc_dmap;
  int wire_combine2_loc_tick;
  int wire_combine2_loc_slack;
  int wire_combine2_loc_error_mode;
  int wire_combine2_loc_utime;
  int wire_combine2_loc_glow_dt;

  Shader wire_combine3_shader;
  int wire_combine3_loc_ema_factor;
  int wire_combine3_loc_prev_circ;
  int wire_combine3_loc_prev_light;
  int wire_combine3_loc_pmap;
  int wire_combine3_loc_segsize;
  int wire_combine3_loc_tick;
  int wire_combine3_loc_slack;
  int wire_combine3_loc_error_mode;
  int wire_combine3_loc_utime;
  int wire_combine3_loc_glow_dt;

  int wire_combine3_loc_tickmod64;
  int wire_combine3_loc_tickgap64;

  Shader nand2_shader;
  int nand2_aloc_pos;
  int nand2_loc_screen_size;

  Shader wire_glow_shader;
  int wire_glow_loc_pmap;
  int wire_glow_loc_segsize;
  int wire_glow_loc_dmap;
  int wire_glow_loc_tick;
  int wire_glow_loc_slack;
  int wire_glow_loc_rtime;

  Shader gaussian_shader;
  int gaussian_loc_size;
  int gaussian_loc_dir;

  Shader bloom_combine_shader;
  int bloom_combine_loc_bloom;
  int bloom_combine_loc_bloom_intensity;
  int bloom_combine_loc_exposure;

  Shader pcomb_shader;
  int pcomb_loc_layer0;
  int pcomb_loc_layer1;
  int pcomb_loc_layer2;
  int pcomb_loc_layer3;
  int pcomb_loc_layer4;
  int pcomb_loc_img_size;
  int pcomb_loc_tgt_size;
  int pcomb_loc_off;
  int pcomb_loc_sp;

  Shader nand4_shader;
  int nand4_aloc_clr;
  int nand4_aloc_pos;
  int nand4_aloc_vert;
  int nand4_loc_w;
  int nand4_loc_h;
  int nand4_loc_mode;

  Shader nandact4_shader;
  int nandact4_aloc_cnand1;
  int nandact4_aloc_cnand2;
  int nandact4_aloc_cnand3;
  int nandact4_aloc_phase;
  int nandact4_aloc_pos;
  int nandact4_aloc_vert;
  int nandact4_loc_w;
  int nandact4_loc_h;

  Shader nandact4err_shader;
  int nandact4err_aloc_pos;
  int nandact4err_aloc_rot;
  int nandact4err_aloc_vert;
  int nandact4err_loc_w;
  int nandact4err_loc_h;
  int nandact4err_loc_utime;

} Shaders;

void shaders_init();
Shaders* get_shaders();

/* V2 API for shaders, to make it easier to use (no need to register everything
 * by hand) */
void shader_load(const char* name);
void shader_vec2(const char* name, Vector2* val);
void shader_vec4(const char* name, Vector4* val);
void shader_ivec2(const char* name, int* val);
void shader_intv(const char* name, int* val, int n);
void shader_tex(const char* name, Texture tex);
int shader_loc(const char* name);
void shader_vec4v(const char* name, Vector4* val, int n);
void shader_unload();

#endif
