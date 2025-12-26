/*
 * Given a distance to the edge, generates a transparency map with 0 on edge
 * and transparent elsewhre.
 */
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
  vec4 c = texture(texture0, fragTexCoord);
  float d = c.r;
  float r = 0.1;
// d=0 --> out, d=1 in, d=0.5 border
#if 0
  float k1 = smoothstep( 0.5 - r, 0.5, d);
  float k2 = smoothstep( 0.5, 0.5+r, d);
  finalColor = (k1 - k2) * vec4(0,0,0,1);
#else
//  float k1 = smoothstep( 0.5 -0.1, 0.5-0.05, d);
//  float k2 = smoothstep( 0.5, 0.5+0.05, d);
//  finalColor = (k1-k2) * vec4(42/255.0,28/255.0,37/255.0,1);

//  float k2 = smoothstep( 0.5, 0.5+0.05, d);
//  finalColor =  (1-k2) * vec4(0, 0,0,1);
#endif

  if (d >= 0.5) {
    finalColor = vec4(0,0,0,0.4);
  } else if (d < 0.5 && d > 0.0) {
    finalColor = vec4(0,0,0,0.3);
  }else if (d == 0) {
    finalColor  = vec4(0,0,0,0.2);
  } else {
    finalColor  = vec4(0);
  }
}

