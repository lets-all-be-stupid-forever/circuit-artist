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
  // vec4 c = texture(texture0, fragTexCoord);
  //vec2 pos = 0.5 * (1 + fragTexCoord);
  vec2 pos = fragTexCoord;

  float kx = 1.0 / tsize.x;
  float ky = 1.0 / tsize.y;
  vec4 ss = vec4(0.0,0.0,0.0,0.0);
  int r = 10;
  float cnt = 0.0;
  for (int y = -r; y < r; y++) {
    for (int x = -r; x < r; x++) {
      vec4 s = texture(texture0, vec2(pos.x + (x+0.5)*kx , pos.y +(y+0.5)*ky));
      // it averages the pixels even if they are black (ie a=0)
      // It's ok for drawing mode, but for sim mode it throws away on/off info
      // if it's done with alphas, so need to use a=1 on simu.
      float krn = 0;
      if (x*x + y*y <= r*r) {
        krn = 1;
      }
      //s.a = 1.0;
      ss = ss + krn * s;
      cnt += krn ;
    }
  }
  vec4 val = ss / cnt;
//  val = togray(val);
//val.rgb = 0.5 * val.rgb;

  finalColor = val;
}
