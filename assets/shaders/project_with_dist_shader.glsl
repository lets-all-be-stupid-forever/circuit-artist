#version 330
/*
 * This shader combines layers using discrete patterns like EDA software do.
 */

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform int distance_type;

// Output fragment color
out vec4 finalColor;

float fmin(float a, float b ) {return a < b ? a : b;}
float dist_max(float aa, float bb){ return max(abs(aa) , abs(bb)); }

uniform vec2 sp; // layer zoom (1 pixel layer = sp pixels screen) (should be an int)
uniform ivec2 off;  // layer offset
uniform ivec2 img_size;  // Layer size

float dist_to_edge_m(vec2 pos, int code) {
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
    float da = dist_max(xc-x, yc-y);
    float db = dist_max(xc+1-x, yc-y);
    float dc = dist_max(xc+1-x, yc+1-y);
    float dd = dist_max(xc-x, yc+1-y);
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
    float da = abs(x-xc);
    float db = abs(y-yc);
    float dc = abs(xc+1-x);
    float dd = abs(yc+1-y);
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

  return dpos - dneg;
}

float square(float x){ return x*x;}
float dist2(float aa, float bb){
  return  aa*aa+bb*bb;
}


float dist_to_edge(vec2 pos, int code) {
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

#if 0 
#define checkpos(r, dx, dy) { \
    df = dist2(dx, dy);
    }
    if (dpos > da) {
       dpos = da;
    }
    dpos = fmin(dpos, da);


vec2 raw_dist_to_edge(vec2 pos, int code) {
  float x = pos.x * img_size.x;
  float y = pos.y * img_size.y;
  float xc = floor(x);
  float yc = floor(y);

  /* First corner distances */
  float dpos = 1000;
  float dneg = 1000;
  float dxmin_pos = 0;
  float dymin_pos = 0;
  float dxmin_neg = 0;
  float dymin_neg = 0;
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
#endif

int get_pixel_code(vec2 pos) {
  float x = pos.x * img_size.x;
  float y = pos.y * img_size.y;
  float xc = floor(x);
  float yc = floor(y);
  int ix = int(floor(xc));
  int iy = int(floor(yc));
  int r = 0;
  int b0 = (texelFetch(texture0, ivec2(ix-1, iy-1), 0).a > 0) ? (1 << 0): 0;
  int b1 = (texelFetch(texture0, ivec2(ix-0, iy-1), 0).a > 0) ? (1 << 1): 0;
  int b2 = (texelFetch(texture0, ivec2(ix+1, iy-1), 0).a > 0) ? (1 << 2): 0;
  int b3 = (texelFetch(texture0, ivec2(ix-1, iy-0), 0).a > 0) ? (1 << 3): 0;
  int b4 = (texelFetch(texture0, ivec2(ix-0, iy-0), 0).a > 0) ? (1 << 4): 0;
  int b5 = (texelFetch(texture0, ivec2(ix+1, iy-0), 0).a > 0) ? (1 << 5): 0;
  int b6 = (texelFetch(texture0, ivec2(ix-1, iy+1), 0).a > 0) ? (1 << 6): 0;
  int b7 = (texelFetch(texture0, ivec2(ix-0, iy+1), 0).a > 0) ? (1 << 7): 0;
  int b8 = (texelFetch(texture0, ivec2(ix+1, iy+1), 0).a > 0) ? (1 << 8): 0;
  return b0 | b1 | b2 | b3 | b4 | b5 | b6| b7 | b8;
}

void main()
{
  vec2 pos = fragTexCoord;
  vec4 clr = texture(texture0, pos);
  int code = get_pixel_code(pos);
  float d;
  if (distance_type == 0) {
    d = dist_to_edge_m(pos, code);
    vec3 rgb = vec3(((-d + 1.0)/ 2.0));
    finalColor = vec4(rgb, 1.0);
  } else if (distance_type == 1) {
    d = dist_to_edge(pos, code);
    vec3 rgb = vec3(((-d + 1.0)/ 2.0));
    finalColor = vec4(rgb, 1.0);
  } 
// else if distance_type == 2) {
//     vec2 rg = raw_dist_to_edge(pos, code);
//     finalColor = vec4(rg.x, rg.y,  0, 1.0);
//   }
}

