#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform sampler2D dmap;
uniform sampler2D pmap;
uniform vec4 colDiffuse;
uniform int tick;
uniform int segsize;
uniform float slack;
uniform float rtime;

int unpack_int32(vec4 rgba) {
  uvec4 bytes = uvec4(rgba * 255.0 + 0.5);  // Denormalize
  uint uval = (bytes.a << 24) | (bytes.b << 16) | 
    (bytes.g << 8) | bytes.r;      // Reassemble bytes
  return int(uval);
}

void main()
{
  vec2 pos = fragTexCoord;
  // slow but for now that will do it
  vec4 c_on = texture(texture0, vec2(pos.x, 1.0 - pos.y));
  vec4 c_off = c_on;
  c_off.rgb = 0.33 * c_on.rgb;
  int p = unpack_int32(texture(pmap, pos));
  int d = unpack_int32(texture(dmap, pos));
  int v0 = p & 3;
  int v1 = (p >> 2)&3;
  p = p >> 4;
  if (p==0) p = -10000;
  int rt = (tick - p) * segsize + int(slack * segsize);
  int dd = (rt - d);

   
  float f = dd / 50.0;
  vec4 fc = vec4(0.0, 0.0, 0.0, 1.0);
  //if (f > 0 && f < 20) fc = vec4(1.0);
  float r = 2.0;
  if (f > 0 && f < rtime) {
    float k = 1.0 - smoothstep(rtime*.5, rtime, f);
    // k is the distance to the tip. 1.0 is on the tip.
    vec4 cc;
    if (v1 == 0) {
    cc = (1.0-k) * c_on + k* c_off;
    } else {
    cc = (1.0-k) * c_off + k* c_on;
    }
    fc = k * cc;


    // if (v1 == 0) {
    // // When v1 is off, it means the wire was ON and is getting off.
    //   fc = fc * 0.33;
    // }
    // fc.a = 1.0;
    // fc.a = vec4(k, k, k, 1.0);
    // fc = vec4(k, k, k, 1.0);
  }
  fc.a = 1.0;
  finalColor = fc ;
}
