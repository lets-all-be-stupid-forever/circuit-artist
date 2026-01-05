#version 330

out vec4 finalColor;
in vec2 txy;
in float f_bugged;

uniform vec4 roi;
uniform sampler2D ref;
uniform int error_mode;
uniform float utime;

in vec4 fc1;
in vec4 fc2;
in vec4 fc3;

vec4 grayscale_darker(vec4 color) {
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  gray *= 0.3;  // Reduce intensity to 30%
  return vec4(vec3(gray), 1.0);  // Fully opaque
}

void main()
{
  // 0-1 --> image texture range.

  float x = roi.z*(txy.x) + roi.x;
  float y = roi.w*(txy.y) + roi.y;

  // float tw = 1024.0;
  // float th = 512.0;;

  // float x = (5.0 * txy.x)/tw + 0.0;
  // float y = (6.0 * txy.y)/th + 352.0/th;
  // float x = txy.x;
  //float y = txy.y;
  vec4 c = texture(ref, vec2(x,y));
  vec4 f = c.r * fc1 + c.g * fc2 + c.b * fc3;
  if (f.a > 0.1) {
    f = f*0.5;
    f.a = 1.0;
  }
  if (error_mode > 0) {
    if (f_bugged < 0.0) {
      // Error state - grayscale darker version
      if (f.a>0.1) {
        f = grayscale_darker(f);
      }
    } else {
      // Non-error state - animated red
      if (f.a>0.1) {
        f = ((utime+1.0)*0.25*0.5 + 0.75) * vec4(1.0, 0.0, 0.0, 1.0);
      }
    }
  }
  finalColor = f;
  //finalColor = vec4(x, y, 0.0, 1.0);
}

