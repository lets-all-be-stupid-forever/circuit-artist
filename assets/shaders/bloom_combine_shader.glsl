#version 330
out vec4 fragColor;
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform sampler2D bloom;
uniform float bloom_intensity;
uniform float exposure;

vec3 reinhard(vec3 color) {
  return color / (1.0 + color);
}
// ACES filmic tone mapping (better for high contrast)
vec3 aces(vec3 x) {
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec4 togray(vec4 c) {
  vec4 color = c;//vec4(r, g, b, a);
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  vec4 grayscale = vec4(gray, gray, gray, color.a);
  return grayscale;
}

vec4 screen(vec4 base, vec4 blend) {
    return vec4(base.rgb + blend.rgb - base.rgb * blend.rgb, 1.0);//blend.a);
}

void main()
{
  vec2 pos = fragTexCoord;
  vec4 scene = texture(texture0, fragTexCoord);
  vec4 glow = texture(bloom, fragTexCoord);

  vec3 s = scene.rgb;
  vec3 linear = pow(s, vec3(2.2));
  vec3 glow2 = pow(glow.rgb, vec3(2.2));

  float brightness = 1;

vec3 hdr = linear * brightness + 3*bloom_intensity * glow2;

//vec3 tonemapped = hdr / (hdr + 1.0);
vec3 tonemapped = aces(hdr);

vec3 final = pow(tonemapped, vec3(1.0 / 2.2));

#if 0
  //vbloom = 2.0*(vbloom - 0.5);
  vbloom = vbloom;

  //vec4 combined = scene + vbloom * bloom_intensity;
  vec4 combined = screen(scene, bloom_intensity*vbloom);
  combined.rgb = aces(combined.rgb);
  combined.a = 1.0;


  // Apply exposure
  //combined *= exposure;

  // Tone map to prevent clipping
  // vec3 mapped = aces(combined);
  // vec3 mapped = reinhard(combined);
  //  vec3 mapped = combined;

  // Optional: add slight contrast
  combined.rgb= pow(combined.rgb, vec3(1.0 / 2.2));  // Gamma correction

  //vec4 clr = vec4(mapped, 1.0);
  //vec4 g = togray(clr);
  //if (g.r<0.1) {
  //  clr.a = 0.0;
  //}
#endif
  fragColor = vec4(final, 1.0);
}
