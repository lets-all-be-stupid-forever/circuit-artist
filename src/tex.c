#include "tex.h"

#include "assert.h"
#include "colors.h"
#include "rlgl.h"
#include "shaders.h"
#include "stdio.h"
#include "stdlib.h"
#include "ui.h"
#include "utils.h"

static struct {
  int cnt;
  Tex* items;
} C = {0};

static bool texsamesize(Tex* a, Tex* b) {
  return (a->w == b->w) && (a->h == b->h);
}

static bool texhassize(Tex* a, int w, int h) {
  return (a->w == w) && (a->h == h);
}

Tex* texnew(int w, int h) {
  Tex* item = C.items;
  while (item) {
    if ((item->refc == 0) && (!item->borrowed) && texhassize(item, w, h)) {
      item->refc++;
      item->uses++;
      return item;
    }
    item = item->next;
  }
  assert(!item);
  C.cnt++;
  if (C.cnt == 50) abort();
  item = calloc(1, sizeof(Tex));
  item->rt = LoadRenderTexture(w, h);
  item->refc = 1;
  item->w = w;
  item->h = h;
  item->uses++;
  item->next = C.items;
  C.items = item;
  return item;
}

void texdel(Tex* t) {
  if (t) t->refc--;
}
Tex* texnewlike(Tex* t) { return texnew(t->w, t->h); }

void texdraw(Tex* dst, Tex* src) {
  assert(dst->w == src->w);
  assert(dst->h == src->h);
  texdraw2(dst->rt, src->rt);
}

void texdraw2(RenderTexture2D dst, RenderTexture2D src) {
  assert(dst.texture.width == src.texture.width);
  assert(dst.texture.height == src.texture.height);
  BeginTextureMode(dst);
  draw_tex(src.texture);
  EndTextureMode();
}

Tex* texgauss(Tex* t) {
  Tex* b1 = texnewlike(t);
  Tex* b2 = texnewlike(t);
  gaussian(1, t->rt.texture, b2->rt);
  gaussian(-1, b2->rt.texture, b1->rt);
  gaussian(1, b1->rt.texture, b2->rt);
  gaussian(-1, b2->rt.texture, b1->rt);
  gaussian(1, b1->rt.texture, b2->rt);
  gaussian(-1, b2->rt.texture, b1->rt);
  texdel(b2);
  return b1;
}

static void texdraw3(RenderTexture2D dst, RenderTexture2D src, Color c) {
  int sw = src.texture.width;
  int sh = src.texture.height;
  int dw = dst.texture.width;
  int dh = dst.texture.height;
  Rectangle r_src = {0, 0, sw, -sh};
  Rectangle r_tgt = {0, 0, dw, dh};
  DrawTexturePro(src.texture, r_src, r_tgt, (Vector2){0, 0}, 0, c);
}

Tex* texgauss2(Tex* t) {
  int w1 = t->rt.texture.width;
  int h1 = t->rt.texture.height;
  int w2 = w1 / 2;
  int h2 = h1 / 2;
  int w3 = w2 / 2;
  int h3 = h2 / 2;
  if (w2 < 1) w2 = 1;
  if (h2 < 1) h2 = 1;
  if (w3 < 1) w3 = 1;
  if (h3 < 1) h3 = 1;
  Tex* b1h = texnew(w1, h1);
  Tex* b1v = texnew(w1, h1);
  Tex* b2h = texnew(w2, h2);
  Tex* b2v = texnew(w2, h2);
  Tex* b3h = texnew(w3, h3);
  Tex* b3v = texnew(w3, h3);
  gaussian(1, t->rt.texture, b1h->rt);
  gaussian(-1, b1h->rt.texture, b1v->rt);
  gaussian(1, b1v->rt.texture, b2h->rt);
  gaussian(-1, b2h->rt.texture, b2v->rt);
  gaussian(1, b2v->rt.texture, b3h->rt);
  gaussian(-1, b3h->rt.texture, b3v->rt);

  BeginTextureMode(b1v->rt);
  BeginBlendMode(BLEND_ADDITIVE);
  // texdraw3(b1v->rt, b2v->rt, (Color){255, 255, 255, 140});
  // texdraw3(b1v->rt, b3v->rt, (Color){255, 255, 255, 100});
  draw_stretched(b2v->rt.texture, b1v->rt, (Color){255, 255, 255, 140});
  draw_stretched(b3v->rt.texture, b1v->rt, (Color){255, 255, 255, 100});
  EndBlendMode();
  EndTextureMode();
  texdel(b1h);
  texdel(b2h);
  texdel(b2v);
  texdel(b3h);
  texdel(b3v);
  return b1v;
}

void texproj(Tex* t, Cam2D cam, Tex* out) {
  BeginTextureMode(out->rt);
  int w = t->rt.texture.width;
  int h = t->rt.texture.height;
  draw_projection_on_target(cam, t->rt.texture, (v2i){w, h}, 0, WHITE);
  EndTextureMode();
}

/* Returns a Tex object that doesnt own its memory */
Tex* texborrow(RenderTexture2D rt) {
  Tex* item = C.items;
  while (item) {
    if ((item->borrowed) && (item->refc == 0) &&
        rt.texture.id == item->rt.texture.id) {
      item->refc++;
      item->uses++;
      return item;
    }
    item = item->next;
  }
  C.cnt++;
  if (C.cnt == 50) abort();
  item = calloc(1, sizeof(Tex));
  item->rt = rt;
  item->refc = 1;
  item->uses = 1;
  item->w = rt.texture.width;
  item->h = rt.texture.height;
  item->next = C.items;
  C.items = item;
  return item;
}

#if 0
void texprerender() {
  Tex* item = C.items;
  while (item) {
    item->uses = 0;
    item = item->next;
  }
}
#endif

void texfree(Tex* t) {
  if (!t->borrowed) UnloadRenderTexture(t->rt);
  C.cnt--;
  free(t);
}

void texcleanup() {
  Tex* prev = NULL;
  Tex* item = C.items;
  int tot = 0;
  int totafter = 0;
  while (item) {
    Tex* next = item->next;
    tot++;
    if (item->uses == 0 && item->refc == 0) {
      if (prev) {
        prev->next = next;
      } else {
        C.items = next;
      }
      texfree(item);
      // printf("cleaned tex\n");
    } else {
      totafter++;
      item->uses = 0;
      prev = item;
    }
    item = next;
  }
  assert(tot < 50);
  // printf("tex_total=%d -> %d\n", tot, totafter);
}

void texclear(Tex* t, Color c) {
  BeginTextureMode(t->rt);
  ClearBackground(c);
  EndTextureMode();
}

void texmapcircuitlight(Texture2D circuit, Tex* pmap, Texture dmap,
                        int error_mode, Times times, Tex** circ, Tex** light) {
  Texture main_tex = circuit;
  int w = circuit.width;
  int h = circuit.height;
  *circ = texnew(w, h);
  *light = texnew(w, h);
  Rectangle source = {0, 0, w, h};
  Rectangle target = {0, 0, w, h};

  rlSetupMRT((*circ)->rt.id, (*light)->rt.texture.id, 2);
  BeginTextureMode((*circ)->rt);
  ClearBackground(BLANK);
  begin_shader(wire_combine2);
  set_shader_tex(wire_combine2, dmap, dmap);
  set_shader_tex(wire_combine2, pmap, pmap->rt.texture);
  set_shader_int(wire_combine2, tick, &times.tick);
  set_shader_int(wire_combine2, error_mode, &error_mode);
  set_shader_float(wire_combine2, slack, &times.slack);
  set_shader_float(wire_combine2, utime, &times.utime);
  set_shader_float(wire_combine2, glow_dt, &times.glow_dt);
  SetTextureFilter(dmap, TEXTURE_FILTER_POINT);
  SetTextureWrap(dmap, TEXTURE_WRAP_CLAMP);
  SetTextureFilter(pmap->rt.texture, TEXTURE_FILTER_POINT);
  SetTextureWrap(pmap->rt.texture, TEXTURE_WRAP_CLAMP);
  SetTextureFilter(main_tex, TEXTURE_FILTER_POINT);
  SetTextureWrap(main_tex, TEXTURE_WRAP_CLAMP);
  DrawTexturePro(main_tex, source, target, (Vector2){0, 0}, 0, WHITE);
  end_shader();
  EndTextureMode();
  rlResetMRT((*circ)->rt.id);
}

Tex* texbloomcombine(Tex* base, Tex* bloom) {
  assert(texsamesize(base, bloom));
  Tex* out = texnewlike(base);
  BeginTextureMode(out->rt);
  begin_shader(bloom_combine);
  float exposure = 1.0;
  float intensity = 1.5;
  set_shader_float(bloom_combine, exposure, &exposure);
  set_shader_float(bloom_combine, bloom_intensity, &intensity);
  set_shader_tex(bloom_combine, bloom, bloom->rt.texture);
  draw_tex(base->rt.texture);
  end_shader();
  EndTextureMode();
  return out;
}

Tex* texupdatelight(Tex* acc, Tex* l1, Tex* m1) {
  assert(texsamesize(acc, l1));
  assert(texsamesize(acc, m1));
  Tex* out = texnewlike(acc);
  BeginTextureMode(out->rt);
  shader_load("update_light");
  shader_tex("tex_l0", acc->rt.texture);
  shader_tex("tex_l1", l1->rt.texture);
  draw_tex(m1->rt.texture);
  shader_unload();
  EndTextureMode();
  return out;
}

void texdrawboard(Tex* t, Cam2D cam, int cw, int ch, Color c) {
  BeginTextureMode(t->rt);
  Color bg_color = get_lut_color(COLOR_DARKGRAY);
  ClearBackground(bg_color);
  // board size
  float cx = cam.off.x - 1e-2;
  float cy = cam.off.y - 1e-2;
  float cs = cam.sp;

  DrawRectangle(cx, cy, cw * cs, ch * cs, c);
  EndTextureMode();
}

void texclock(Tex* t, float mx, float my) {
  BeginTextureMode(t->rt);
  shader_load("clock");
  Texture tex = ui_get_sprites();
  Rectangle source = {
      0,
      0,
      tex.width,
      tex.height,
  };
  int tw = t->rt.texture.width;
  int th = t->rt.texture.height;
  Rectangle dest = {
      0,
      0,
      tw,
      th,
  };

  Vector2 target_size = {(float)tw, (float)th};
  Vector2 mouse = {mx, my};
  shader_vec2("target_size", &target_size);
  shader_vec2("mouse", &mouse);

  DrawTexturePro(tex, source, dest, (Vector2){0, 0}, 0, WHITE);
  shader_unload();
  EndTextureMode();
}

// Tex* texgenoverlay(Texture chip) {
// }
//
//

Tex* texaffine(Tex* x, Tex* y, float a, float b, float c) {
  assert(x->rt.texture.width == y->rt.texture.width);
  assert(x->rt.texture.height == y->rt.texture.height);
  Tex* out = texnewlike(x);
  BeginTextureMode(out->rt);
  shader_load("tex_affine");
  shader_tex("tex_y", y->rt.texture);
  Vector2 f_a = {a, a};
  Vector2 f_b = {b, b};
  Vector2 f_c = {c, c};
  shader_vec2("f_a", &f_a);
  shader_vec2("f_b", &f_b);
  shader_vec2("f_c", &f_c);
  draw_tex(x->rt.texture);
  shader_unload();
  EndTextureMode();
  return out;
}
