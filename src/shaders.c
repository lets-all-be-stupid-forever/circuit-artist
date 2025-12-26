#include "shaders.h"

#include "assert.h"
#include "stb_ds.h"
#include "stdio.h"

#define SHADER_LOC(a, b)                                   \
  {                                                        \
    _s.a##_loc_##b = GetShaderLocation(_s.a##_shader, #b); \
    /*assert(_s.a##_loc_##b != -1);*/                      \
  }
#define SHADER_ALOC(a, b)                                         \
  {                                                               \
    _s.a##_aloc_##b = GetShaderLocationAttrib(_s.a##_shader, #b); \
    /*assert(_s.a##_aloc_##b != -1);*/                            \
  }
#define SHADER_LOAD(name)                                         \
  {                                                               \
    _s.name##_shader =                                            \
        LoadShader(0, "../assets/shaders/" #name "_shader.glsl"); \
    assert(IsShaderValid(_s.name##_shader));                      \
  }

#define SHADER_LOAD2(name)                                                  \
  {                                                                         \
    _s.name##_shader = LoadShader("../assets/shaders/" #name "_vert.glsl",  \
                                  "../assets/shaders/" #name "_frag.glsl"); \
    assert(IsShaderValid(_s.name##_shader));                                \
  }

static Shaders _s = {0};

void shaders_init() {
  SHADER_LOAD(fill);
  SHADER_LOAD(rotate);
  SHADER_LOC(rotate, ccw);

  SHADER_LOAD(comb);
  SHADER_LOC(comb, dst_tex);
  // SHADER_LOC(comb, off);
  SHADER_LOC(comb, src_size);
  SHADER_LOC(comb, dst_size);
  //   SHADER_LOC(comb, roi_size);
  SHADER_LOC(comb, src_off);
  SHADER_LOC(comb, dst_off);

  SHADER_LOAD(project);
  SHADER_LOC(project, sp);
  SHADER_LOC(project, img_size);

  SHADER_LOAD(project_pattern);
  SHADER_LOC(project_pattern, sp);
  SHADER_LOC(project_pattern, img_size);
  SHADER_LOC(project_pattern, pattern);
  SHADER_LOC(project_pattern, pad);

  SHADER_LOAD(thumbnail);
  SHADER_LOC(thumbnail, sp);
  SHADER_LOC(thumbnail, img_size);

  SHADER_LOAD(update);
  SHADER_LOC(update, img_size);
  SHADER_LOC(update, sel_size);
  SHADER_LOC(update, sel_off);
  SHADER_LOC(update, sel);
  SHADER_LOC(update, tool_size);
  SHADER_LOC(update, tool_off);
  SHADER_LOC(update, tool);

  SHADER_LOAD(selrect);
  SHADER_LOC(selrect, rsize);
  SHADER_LOC(selrect, rect_pos);
  SHADER_LOC(selrect, pattern_shift);
  SHADER_LOC(selrect, pattern_width);

  SHADER_LOAD2(wire);
  SHADER_LOC(wire, slack);
  SHADER_LOC(wire, pulses);
  SHADER_LOC(wire, w);
  SHADER_LOC(wire, h);
  SHADER_LOC(wire, mvp);
  SHADER_LOC(wire, pulse_w);
  SHADER_LOC(wire, cur_tick);
  SHADER_LOC(wire, ref);
  SHADER_LOC(wire, dmap);
  SHADER_LOC(wire, utime);
  SHADER_LOC(wire, error_mode);
  SHADER_ALOC(wire, vert);
  SHADER_ALOC(wire, pos);
  SHADER_ALOC(wire, wid);

  SHADER_LOAD2(nand);
  SHADER_ALOC(nand, c1);
  SHADER_ALOC(nand, c2);
  SHADER_ALOC(nand, c3);
  SHADER_ALOC(nand, bugged);
  SHADER_LOC(nand, error_mode);
  SHADER_LOC(nand, mvp);
  SHADER_LOC(nand, utime);
  SHADER_LOC(nand, ref);
  SHADER_LOC(nand, roi);
  SHADER_ALOC(nand, vert);
  SHADER_ALOC(nand, pos);

  SHADER_LOAD2(pixel_error);
  SHADER_ALOC(pixel_error, vert);
  SHADER_ALOC(pixel_error, pos);
  SHADER_LOC(pixel_error, utime);
  SHADER_LOC(pixel_error, mvp);
  SHADER_LOC(pixel_error, zoom);

  SHADER_LOAD(wire_combine);
  SHADER_LOC(wire_combine, dmap);
  SHADER_LOC(wire_combine, segsize);
  SHADER_LOC(wire_combine, pmap);
  SHADER_LOC(wire_combine, tick);
  SHADER_LOC(wire_combine, slack);
  SHADER_LOC(wire_combine, error_mode);
  SHADER_LOC(wire_combine, utime);

  SHADER_LOAD(wire_combine2);
  SHADER_LOC(wire_combine2, dmap);
  SHADER_LOC(wire_combine2, segsize);
  SHADER_LOC(wire_combine2, pmap);
  SHADER_LOC(wire_combine2, tick);
  SHADER_LOC(wire_combine2, slack);
  SHADER_LOC(wire_combine2, error_mode);
  SHADER_LOC(wire_combine2, utime);
  SHADER_LOC(wire_combine2, glow_dt);

  SHADER_LOAD(wire_glow);
  SHADER_LOC(wire_glow, dmap);
  SHADER_LOC(wire_glow, segsize);
  SHADER_LOC(wire_glow, pmap);
  SHADER_LOC(wire_glow, tick);
  SHADER_LOC(wire_glow, rtime);
  SHADER_LOC(wire_glow, slack);

  SHADER_LOAD(gaussian);
  SHADER_LOC(gaussian, size);
  SHADER_LOC(gaussian, dir);

  SHADER_LOAD(bloom_combine);
  SHADER_LOC(bloom_combine, bloom);
  SHADER_LOC(bloom_combine, bloom_intensity);
  SHADER_LOC(bloom_combine, exposure);

  SHADER_LOAD(pcomb);
  SHADER_LOC(pcomb, layer0);
  SHADER_LOC(pcomb, layer1);
  SHADER_LOC(pcomb, layer2);
  SHADER_LOC(pcomb, layer3);
  SHADER_LOC(pcomb, layer4);
  SHADER_LOC(pcomb, sp);
  SHADER_LOC(pcomb, off);
  SHADER_LOC(pcomb, img_size);
  SHADER_LOC(pcomb, tgt_size);
}

typedef struct {
  Shader shader;
  struct {
    char* key;
    int value;
  } * locs;
} ShaderDef;

static struct {
  int active;
  struct {
    char* key;
    ShaderDef value;
  } * registry;
} C = {0};

Shaders* get_shaders() { return &_s; }

void shader_load(const char* name) {
  int i = shgeti(C.registry, name);
  if (i == -1) {
    ShaderDef sd = {0};
    sd.shader =
        LoadShader(0, TextFormat("../assets/shaders/%s_shader.glsl", name));
    assert(IsShaderValid(sd.shader));
    shput(C.registry, name, sd);
    i = shgeti(C.registry, name);
  }
  C.active = i;
  BeginShaderMode(C.registry[C.active].value.shader);
}

int shader_loc(const char* name) {
  int i = shgeti(C.registry[C.active].value.locs, name);
  if (i >= 0) {
    return C.registry[C.active].value.locs[i].value;
  }
  int loc = GetShaderLocation(C.registry[C.active].value.shader, name);
  if (loc == -1) {
    fprintf(stderr, "Couldn't find loc: [%s] %s.\n", C.registry[C.active].key,
            name);
  }
  printf("act=%d name=%s loc=%d\n", C.active, name, loc);
  // assert(loc != -1);
  shputi(C.registry[C.active].value.locs, name, loc);
  return loc;
}

void shader_unload() {
  C.active = -1;
  EndShaderMode();
}

void shader_vec2(const char* name, Vector2* val) {
  int loc = shader_loc(name);
  SetShaderValue(C.registry[C.active].value.shader, loc, val,
                 SHADER_UNIFORM_VEC2);
}

void shader_vec4(const char* name, Vector4* val) {
  int loc = shader_loc(name);
  SetShaderValue(C.registry[C.active].value.shader, loc, val,
                 SHADER_UNIFORM_VEC4);
}

void shader_ivec2(const char* name, int* val) {
  int loc = shader_loc(name);
  SetShaderValue(C.registry[C.active].value.shader, loc, val,
                 SHADER_UNIFORM_IVEC2);
}

void shader_intv(const char* name, int* val, int n) {
  int loc = shader_loc(name);
  SetShaderValueV(C.registry[C.active].value.shader, loc, val,
                  SHADER_UNIFORM_INT, n);
}

void shader_tex(const char* name, Texture tex) {
  int loc = shader_loc(name);
  SetShaderValueTexture(C.registry[C.active].value.shader, loc, tex);
}

void shader_vec4v(const char* name, Vector4* val, int n) {
  int loc = shader_loc(name);
  SetShaderValueV(C.registry[C.active].value.shader, loc, val,
                  SHADER_UNIFORM_VEC4, n);
}

