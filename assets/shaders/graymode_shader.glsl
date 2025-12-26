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

vec4 get_pixel(sampler2D layer, int l) {
  vec2 pos = fragTexCoord;
  int spi = int(sp);
  int px = int(pos.x * img_size.x * sp.x + off.x);
  int py = int(pos.y * img_size.y * sp.y + off.y);
  return texture(layer, pos);
}

vec4 togray(vec4 c) {
  vec4 color = c;//vec4(r, g, b, a);
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  vec4 grayscale = vec4(gray, gray, gray, color.a);
  return grayscale;
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
  if (l!=al) {
    if (c1.a == 0) {
      float f = 0.3;
      // if (l==0) c1.rgb = vec3(f,0,0);
      // if (l==1) c1.rgb = vec3(0.0,f,0);
      // if (l==2) c1.rgb = vec3(0.0,0,f);
      c1.rgb = vec3(f);
    } else {
      c1 = vec4(0.1,0.1,0.1,1);
    }
    if (l == al - 1) c1.a = 0.5;
    if (l == al - 2) c1.a = 0.25;
    if (l <= al - 3) c1.a = 0.0;
  }

  if (l > al) {
    c1= vec4(0);
  }
  return c1;
}

vec4 blend(vec4 source, vec4 destination, int l) {
  // Simple alpha blending (source over destination)
  float alpha = source.a + destination.a * (1.0 - source.a);
  vec3 color = (source.rgb * source.a + destination.rgb * destination.a * (1.0 - source.a)) / alpha;
  return vec4(color, alpha);
}

void main()
{
  vec4 c = vec4(0,0,0,1);
  vec4 c0 = drawlayer(0);
  vec4 c1 = drawlayer(1);
  vec4 c2 = drawlayer(2);
  vec4 c3 = drawlayer(3);
  if (c0.a > 0 && layer_mode[0] != -2) c = blend(c0, c, 0);
  if (c1.a > 0 && layer_mode[1] != -2) c = blend(c1, c, 1);
  if (c2.a > 0 && layer_mode[2] != -2) c = blend(c2, c, 2);
  if (c3.a > 0 && layer_mode[3] != -2) c = blend(c3, c, 3);
  finalColor = c;
}





