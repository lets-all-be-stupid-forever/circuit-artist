#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

uniform vec2 sp; // zoom
uniform ivec2 img_size; // image size in image pixels

vec4 msample(vec2 pos, int r) {
  float kx = 1.0 / float(img_size.x);
  float ky = 1.0 / float(img_size.y);
  vec4 ss = vec4(0.0,0.0,0.0,0.0);
  float cnt = 0.0;
  for (int y = -r; y < r; y++) {
    for (int x = -r; x < r; x++) {
      vec4 s = texture(texture0, vec2(pos.x + (x+0.5)*kx , pos.y +(y+0.5)*ky));
      // if (s.a > 0.01) {
      if (true) {
        s.a = 1.0;
        ss = ss + s;
        cnt += 1.0;
      }
    }
  }
  return ss / cnt;
}

void main()
{
  // Texel color fetching from texture sampler
  vec2 pos = fragTexCoord;
  vec4 img_color;
  if (sp.x > 1.0 - 1e-2) {
    img_color = texture(texture0, pos);
  } else if (sp.x > 0.5 - 1e-2) {
    img_color = msample(pos, 1);
  } else if (sp.x > 0.25 - 1e-2) {
    img_color = msample(pos, 2);
  } else {
    img_color = msample(pos, 4);
  }
  finalColor =  img_color * colDiffuse *fragColor;
}
