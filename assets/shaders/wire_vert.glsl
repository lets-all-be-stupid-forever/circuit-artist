#version 330

in vec3 vert;
in vec4 pos;
in float wid;

uniform sampler2D pulses;

uniform int pulse_w;
uniform int cur_tick;
uniform float slack;
uniform int w;
uniform int h;
uniform mat4 mvp;

out vec2 fragTexCoord;
out vec4 pvalue;
out float ori;

vec4 pack_int32(int value) {
  uint uval = uint(value);  // Reinterpret as unsigned
  uvec4 bytes = uvec4(
                      uval & 0xFFu,         // r: lowest byte
                      (uval >> 8) & 0xFFu,  // g: second byte
                      (uval >> 16) & 0xFFu, // b: third byte
                      (uval >> 24) & 0xFFu  // a: highest byte
                     );
  return vec4(bytes) / 255.0;  // Normalize to [0, 1]
}

vec4 mygetnum() {
  int off = floatBitsToInt(wid);
  if (off < 0) {
    off = -off -1;
    ori = 1.0;
  } else {
    ori = 0.0;
  }
  int px = off % pulse_w;
  int py = off / pulse_w;
  ivec2 coord = ivec2(px, py);
  return texelFetch(pulses, coord, 0);
}

void main()
{
    vec4 p = pos; 
    float x0 = p.x;
    float y0 = p.y;
    float x1 = p.z;
    float y1 = p.w;
    float vx = vert.x;
    float vy = vert.y;
    float x = x0 * (1.0 -vx) + vx * x1;
    float y = y0 * (1.0 -vy) + vy * y1;
    float tx = round(x - 0.5);
    float ty = round(y - 0.5);
    int s = 0;
    pvalue = mygetnum();
    float xx = (x)/w;
    float yy = 1.0 - (y)/h;
    fragTexCoord = vec2(xx, 1.0-yy);
    x = 2.0*x/w-1.0;
    y = 2.0*y/h-1.0;
    gl_Position = mvp * vec4(x,  y, 0.0, 1.0);
}
