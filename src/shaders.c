
#include "shaders.h"

#include "stdio.h"
#define SHADER_LOC(a, b) _s.a##_loc_##b = GetShaderLocation(_s.a##_shader, #b);
#define SHADER_LOAD(name) \
  _s.name##_shader = LoadShader(0, "../assets/shaders/" #name "_shader.glsl");

static Shaders _s = {0};

void InitShaders() {
  SHADER_LOAD(fill);
  SHADER_LOAD(rotate);
  SHADER_LOC(rotate, ccw);

  SHADER_LOAD(comb);
  SHADER_LOC(comb, dst_tex);
  SHADER_LOC(comb, off);
  SHADER_LOC(comb, src_size);
  SHADER_LOC(comb, dst_size);
  SHADER_LOC(comb, roi_size);
  SHADER_LOC(comb, src_off);
  SHADER_LOC(comb, dst_off);

  SHADER_LOAD(project);
  SHADER_LOC(project, sp);
  SHADER_LOC(project, img_size);
  SHADER_LOC(project, tpl);

  SHADER_LOAD(update);
  SHADER_LOC(update, img_size);
  SHADER_LOC(update, sel_size);
  SHADER_LOC(update, sel_off);
  SHADER_LOC(update, sel);
  SHADER_LOC(update, tool_size);
  SHADER_LOC(update, tool_off);
  SHADER_LOC(update, tool);
  SHADER_LOC(update, mode);
  SHADER_LOC(update, simu_state);
  SHADER_LOC(update, bg_color);
  SHADER_LOC(update, bugged_color);
  SHADER_LOC(update, undefined_color);
  SHADER_LOC(update, comp_x);
  SHADER_LOC(update, comp_y);
  SHADER_LOC(update, state_buf);
  SHADER_LOC(update, prev_state_buf);
  SHADER_LOC(update, prev_state_f);
  SHADER_LOC(update, unchanged_alpha);

  SHADER_LOAD(selrect);
  SHADER_LOC(selrect, rsize);
  SHADER_LOC(selrect, rect_pos);
  SHADER_LOC(selrect, pattern_shift);
  SHADER_LOC(selrect, pattern_width);
}

Shaders* GetShaders() { return &_s; }
