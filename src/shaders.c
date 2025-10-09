
#include "shaders.h"

#include "assert.h"
#include "stdio.h"
#define SHADER_LOC(a, b) _s.a##_loc_##b = GetShaderLocation(_s.a##_shader, #b);
#define SHADER_LOAD(name)                                         \
  {                                                               \
    _s.name##_shader =                                            \
        LoadShader(0, "../assets/shaders/" #name "_shader.glsl"); \
    assert(IsShaderReady(_s.name##_shader));                      \
  }

#define SHADER_LOAD_CODE(name)                               \
  {                                                          \
    printf("Loading shader %s\n", #name);                    \
    _s.name##_shader = LoadShaderFromMemory(0, name##_code); \
    assert(IsShaderReady(_s.name##_shader));                 \
  }

static Shaders _s = {0};

static const char selrect_code[] =
    "#version 330                                           \n "
    "in vec2 fragTexCoord;                                  \n "
    "in vec4 fragColor;                                     \n "
    "uniform vec4 colDiffuse;                               \n "
    "uniform vec2 rsize;                                    \n "
    "uniform vec2 rect_pos;                                 \n "
    "uniform int pattern_shift;                             \n "
    "uniform int pattern_width;                             \n "
    " out vec4 finalColor;                                  \n "
    "void main() {                                          \n "
    "  int fx = int(round(fragTexCoord.x * rsize.x - 0.5)); \n "
    "  int fy = int(round(fragTexCoord.y * rsize.y - 0.5)); \n "
    "  int ix = int(round(rect_pos.x));                     \n "
    "  int iy = int(round(rect_pos.y));                     \n "
    "  int p = ix + fx + iy + fy + pattern_shift;           \n "
    "  p = p % (2 * pattern_width);                         \n "
    "  if (p < pattern_width) {                             \n "
    "    finalColor = fragColor * vec4(1.0, 1.0, 1.0, 1.0); \n "
    "  } else {                                             \n "
    "    finalColor = fragColor * vec4(0.0, 0.0, 0.0, 1.0); \n "
    "  }                                                    \n "
    "}                                                      \n ";

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

  SHADER_LOAD_CODE(selrect);
  SHADER_LOC(selrect, rsize);
  SHADER_LOC(selrect, rect_pos);
  SHADER_LOC(selrect, pattern_shift);
  SHADER_LOC(selrect, pattern_width);

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
}

Shaders* GetShaders() { return &_s; }
