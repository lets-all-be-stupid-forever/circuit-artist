
#version 330

in vec2 fragTexCoord;
uniform vec2 tsize;

out vec4 finalColor;
uniform sampler2D texture0;

vec4 togray(vec4 c) {
  vec4 color = c;//vec4(r, g, b, a);
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  vec4 grayscale = vec4(gray, gray, gray, color.a);
  return grayscale;
}


void main() {
  vec4 ref = texture(texture0, fragTexCoord);
  //vec2 pos = 0.5 * (1 + fragTexCoord);
  vec2 pos = fragTexCoord;

  float kx = 1.0 / tsize.x;
  float ky = 1.0 / tsize.y;

vec2 size_factor = vec2(kx, ky);
    const int range = 1; // should be = (samples - 1)/2;
    vec4 sum = vec4(0);
    int nbg = 0;
    int nc = 0;
    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            vec4 s = texture(texture0, fragTexCoord + vec2(x, y)*size_factor);
            if (s.a > 0.5) {
              nc = nc + 1;
            } else {
              nbg = nbg + 1;
            }
        }
    }
if (ref.a == 0) {
    vec4 c = vec4(0,0,0,1);
    if (nc == 0) {
        c = vec4(0.0, 0.0, 0.0, 0.0);
    }
    if (nbg == 0) {
        c = vec4(0.0, 0.0, 0.0, 0.0);
    }
    finalColor = c;
} else {
    finalColor = vec4(0);;
}
}
