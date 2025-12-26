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
uniform int error_mode;
uniform float utime;

int unpack_int32(vec4 rgba) {
  uvec4 bytes = uvec4(rgba * 255.0 + 0.5);  // Denormalize
  uint uval = (bytes.a << 24) | (bytes.b << 16) |
    (bytes.g << 8) | bytes.r;      // Reassemble bytes
  return int(uval);
}

vec4 grayscale_darker(vec4 color) {
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  gray *= 0.3;  // Reduce intensity to 30%
  return vec4(vec3(gray), 1.0);  // Fully opaque
}

void main()
{
  vec2 pos = fragTexCoord;
  vec2 p2 = vec2(pos.x, 1.0 - pos.y);
  vec4 c = texture(texture0, p2);
  int p = unpack_int32(texture(pmap, pos));
  int d = unpack_int32(texture(dmap, pos));

  // p = Activation time
  int v0 = p & 3;
  int v1 = (p >> 2)&3;
  p = p >> 4;
  if (p==0) p = -10000;

  // Releative time.
  int rt = (tick - p) * segsize + int(slack * segsize);

  int dd = (rt - d);
  // if (b == 51) {
  //   finalColor = vec4(0, 1, 0, 1.0);
  // } else {
  //   finalColor = vec4(1, 0, 0, 1.0);
  // }
  float f = dd / 50.0;
  int v = v0;
  if (f > 0) {
    v = v1;
  }
 //if (d > 2) v = 1;

  vec4 fc = c;

  if (error_mode == 1) {
    // Error mode: red for v=2 (errors), grayscale_darker for v=0 (off), background stays background
    if (c.a == 0.0) {
      // Background pixel - keep it transparent
      fc = c;
    } else if (v == 2) {
      // Error state - animated red
      fc = ((utime+1.0)*0.25*0.5 + 0.75) * vec4(1.0, 0.0, 0.0, 1.0);
    } else if (v == 0) {
      // Off state - grayscale darker version of original color
      fc = grayscale_darker(c);
    } else {
      // v == 1, keep original wire color
      fc = c;
    }
  } else {
    // Normal mode - original behavior
    float alpha = 1.0;
    if (v == 0) alpha = 0.33; //160.0 / 255.0; // When bit is off, it's darker
    if (v == 1) alpha = 1.0; // 255.0 / 255.0;// When bit is oon, it's brighter
    if (v == 2) alpha = -1.0;//

    if (abs(alpha + 1.0) < 1e-2){
        fc = vec4(1.0, 0.0, 1.0, 1.0);
    } else {
      fc = fc * alpha;
      fc.a = 1.0;
    }
    fc.a = c.a;
  }
  finalColor = fc ;

#if 0
  //float f = (tick - b)/50.0;
  float f = (b)/20.0;
  img_color = img_color * f;
  // img_color.g = 1.0;
  finalColor = vec4(f, f, f, 1.0);
#endif
}
