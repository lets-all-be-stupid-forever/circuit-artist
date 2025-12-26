#version 330
/*
 * This shader combines layers using discrete patterns like EDA software do.
 */

in vec2 fragTexCoord;
in vec4 fragColor;
// Input uniform values
uniform sampler2D layer0;
uniform sampler2D layer1;
uniform sampler2D layer2;
uniform sampler2D layer3;
uniform sampler2D layer4;
uniform vec4 colDiffuse;

//  16k max. 32x32 = 1k

uniform sampler2D pat_tex;
uniform ivec2 pat_tex_size;
uniform ivec2 pattern0;
uniform int nl;
uniform int al;

uniform vec4 layer_color[4];
uniform int layer_pat[4];
uniform int layer_rad[4];
uniform int layer_mode[4];

// Output fragment color
out vec4 finalColor;

uniform vec2 sp; // layer zoom (1 pixel layer = sp pixels screen) (should be an int)
uniform ivec2 off;  // layer offset
uniform ivec2 img_size;  // Layer size
uniform ivec2 tgt_size;  // Dest buffer size

float fmin(float a, float b ) {return a < b ? a : b;}

float square(float x){ return x*x;}

float dist2(float aa, float bb){
  return  aa*aa+bb*bb;
}

bool isvia(int code) {
  int n = ((code >> 1)&1) + ((code >> 3)&1) +((code >> 5)&1) + ((code >> 7)&1);
  int c = (code >> 4)&1;
  return (c == 1) && (n <= 1);
}

/* Code:
 *   bit0  bit1  bit2
 *   bit3  bit4  bit5
 *   bit6  bit7  bit8 */
int get_pixel_code(sampler2D layer) {
  vec2 pos = fragTexCoord;
  float x = pos.x * img_size.x;
  float y = pos.y * img_size.y;
  float xc = floor(x);
  float yc = floor(y);
  int ix = int(floor(xc));
  int iy = int(floor(yc));
  int r = 0;
  int b0 = (texelFetch(layer, ivec2(ix-1, iy-1), 0).a > 0) ? (1 << 0): 0;
  int b1 = (texelFetch(layer, ivec2(ix-0, iy-1), 0).a > 0) ? (1 << 1): 0;
  int b2 = (texelFetch(layer, ivec2(ix+1, iy-1), 0).a > 0) ? (1 << 2): 0;
  int b3 = (texelFetch(layer, ivec2(ix-1, iy-0), 0).a > 0) ? (1 << 3): 0;
  int b4 = (texelFetch(layer, ivec2(ix-0, iy-0), 0).a > 0) ? (1 << 4): 0;
  int b5 = (texelFetch(layer, ivec2(ix+1, iy-0), 0).a > 0) ? (1 << 5): 0;
  int b6 = (texelFetch(layer, ivec2(ix-1, iy+1), 0).a > 0) ? (1 << 6): 0;
  int b7 = (texelFetch(layer, ivec2(ix-0, iy+1), 0).a > 0) ? (1 << 7): 0;
  int b8 = (texelFetch(layer, ivec2(ix+1, iy+1), 0).a > 0) ? (1 << 8): 0;
  return b0 | b1 | b2 | b3 | b4 | b5 | b6| b7 | b8;
}


float dist_to_edge(int code) {
  vec2 pos = fragTexCoord;
  float x = pos.x * img_size.x;
  float y = pos.y * img_size.y;
  float xc = floor(x);
  float yc = floor(y);

  /* First corner distances */
  float dpos = 1000;
  float dneg = 1000;
  int ix = int(floor(xc));
  int iy = int(floor(yc));
  {
    int a = (code >> 0) & 1;
    int b = (code >> 2) & 1;
    int c = (code >> 8) & 1;
    int d = (code >> 6) & 1;
    float da = dist2(xc-x, yc-y);
    float db = dist2(xc+1-x, yc-y);
    float dc = dist2(xc+1-x, yc+1-y);
    float dd = dist2(xc-x, yc+1-y);
    if (a == 1) dpos = fmin(dpos, da);
    if (b == 1) dpos = fmin(dpos, db);
    if (c == 1) dpos = fmin(dpos, dc);
    if (d == 1) dpos = fmin(dpos, dd);
    if (a == 0) dneg = fmin(dneg, da);
    if (b == 0) dneg = fmin(dneg, db);
    if (c == 0) dneg = fmin(dneg, dc);
    if (d == 0) dneg = fmin(dneg, dd);
  }
  /* Lateral distances */
  /* Code:
   *   bit0 >bit1  bit2
   *   bit3  bit4  bit5
   *   bit6  bit7  bit8 */
  {
    int a = (code >> 3) & 1;
    int b = (code >> 1) & 1;
    int c = (code >> 5) & 1;
    int d = (code >> 7) & 1;
    float da = square(x-xc);
    float db = square(y-yc);
    float dc = square(xc+1-x);
    float dd = square(yc+1-y);
    if (a == 1) dpos = fmin(dpos, da);
    if (b == 1) dpos = fmin(dpos, db);
    if (c == 1) dpos = fmin(dpos, dc);
    if (d == 1) dpos = fmin(dpos, dd);
    if (a == 0) dneg = fmin(dneg, da);
    if (b == 0) dneg = fmin(dneg, db);
    if (c == 0) dneg = fmin(dneg, dc);
    if (d == 0) dneg = fmin(dneg, dd);
  }
  {
    if (((code >> 4)&1) != 0) {
      dpos = 0;
      dneg = fmin(dneg, 1);
    } else {
      dneg= 0;
      dpos= fmin(dpos, 1);
    }
  }

  return sqrt(dpos) - sqrt(dneg);
}

vec4 togray(vec4 c) {
  vec4 color = c;//vec4(r, g, b, a);
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  vec4 grayscale = vec4(gray, gray, gray, color.a);
  return grayscale;
}

// 0 is the first outside.
// -1 is the first pixel inside

float get_pattern2(int px, int py, float d, int p) {
  if (p == -1) return 1;
  px = px / 2;
  py = py / 2;
  px = (px % 64);
  py = 63 - (py % 64);
  int cx = 608 + px;
  int cy = 384 + py;
  vec4 c = texelFetch(pat_tex, ivec2(cx, cy), 0);

  // float dd = smoothstep(0, 0.2, -d);
  //float dd = smoothstep(-0.4, -0.1, d);
  //float dd2 = smoothstep(-0.1, -0.0, d);
  float t = 0.1;
  // d --> -0.5 in middle, 0 in border.
  // what i want?
  // 1 in border - t
  // 0 in border, 0 in middle
  // smoothstep(a, b, x) ==> 0 for x<a, 1 for x > b
  // a, b, c --> 0 x<a, x=1 in b, 0 for x>c
  // smoothstep(a, b) - smoothstep(b, c)
  float a = 0;
  float b = 0.1;
  float cc = 0.2;
  float x = -d;
  float dd = smoothstep(a, b, x) - smoothstep(b,cc, x);

dd = dd + 0.5 * smoothstep(0.1, 0.2, x);

  return dd;
}

int get_pattern(int px, int py, int p) {
//  if (sp.x > 6.0) {
//    px = px/2;
//    py = py/2;
//  }
  if (p == -1) return 1;
  px = (px % 16);
  py = 15 - (py % 16);
  vec4 c = texelFetch(pat_tex, ivec2(px+pattern0.x, py+pattern0.y + 16*p), 0);
  if (c.a > 0) return 1;
  return 0;
}

vec4 via_formula(vec4 c) {
  vec2 pos = fragTexCoord;
  float x = pos.x * img_size.x;
  float y = pos.y * img_size.y;
  float xc = floor(x);
  float yc = floor(y);

  /* First corner distances */
  int ix = int(floor(xc));
  int iy = int(floor(yc));
  xc = x - ix;
  yc = y - iy;

  float ox = (608.0 + xc * 16.0)/ 1024.0;
  float oy = (160.0 + yc * 16.0)/ 512.0;
  vec4 r = texture(pat_tex, vec2(ox, oy));
  vec4 k = r.a * r + (1-r.a) * c;
  return k;
}

vec4 get_pixel(sampler2D layer, int l) {
  vec2 pos = fragTexCoord;
  int spi = int(sp);
  int px = int(pos.x * img_size.x * sp.x + off.x);
  int py = int(pos.y * img_size.y * sp.y + off.y);
  vec4 img_color1 = texture(layer, pos);
  int code = get_pixel_code(layer);
  float d = dist_to_edge(code);
  bool via = isvia(code) && l > 0;
  int di = int(d * spi) + (d>0?0:-1);
  float p = get_pattern2(px, py, d, layer_pat[l]);
  di += layer_rad[l];
  int mode = layer_mode[l];
  if (mode == -2) {
    img_color1 = vec4(0);
  }
  if (mode == 0) {
    img_color1 = p*img_color1;
 }
  if (mode == 1) {
    img_color1 = p*layer_color[l];
  }
  if (mode == 2) {
     img_color1 = togray(img_color1);
  }
  //img_color1.a = layer_color[l].a;
  // if (di<-1) {
  //   img_color1 = p*img_color1;
  // }
  if (di >= 0) img_color1 = vec4(0,0,0,0.0);

  if (via) {
    // TODO sample from via texture or something
    img_color1 = via_formula(img_color1);
  }

  return img_color1;
}

vec4 blend(vec4 source, vec4 destination) {
  // Simple alpha blending (source over destination)
  float alpha = source.a + destination.a * (1.0 - source.a);
  vec3 color = (source.rgb * source.a + destination.rgb * destination.a * (1.0 - source.a)) / alpha;
  return vec4(color, alpha);
}

vec4 drawlayer(int l) {
  if (l < 0 || l >= nl) {
    return vec4(0);
  }
  vec4 c1;
  if (l == 0) c1 = get_pixel(layer0, l);
  if (l == 1) c1 = get_pixel(layer1, l);
  if (l == 2) c1 = get_pixel(layer2, l);
  if (l == 3) c1 = get_pixel(layer3, l);
  if (l == 4) c1 = get_pixel(layer4, l);
  return c1;
}

vec4 blend2(vec4 c1, vec4 c, int l) {
  return blend(c1, c);
  if (l <= al) {
  return blend(c1, c);
  } else {
    if (c1.a>0.0 && c.a>0.9)  return vec4(0,0,0,1);
    return blend(c1, c);
  }
}

void main()
{
  vec4 c = vec4(0,0,0,1);
  vec4 c0 = drawlayer(0);
  vec4 c1 = drawlayer(1);
  vec4 c2 = drawlayer(2);
  vec4 c3 = drawlayer(3);
  if (c0.a > 0 && layer_mode[0] != -2) c = blend2(c0, c, 0);
  if (c1.a > 0 && layer_mode[1] != -2) c = blend2(c1, c, 1);
  if (c2.a > 0 && layer_mode[2] != -2) c = blend2(c2, c, 2);
  if (c3.a > 0 && layer_mode[3] != -2) c = blend2(c3, c, 3);

  // Step1: edge - only.

//  vec4 img_color2 = texture(layer1, pos);
  finalColor = c;
}


