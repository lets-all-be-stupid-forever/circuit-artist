#version 330

in vec2 fpos;
flat in float f_rot;

out vec4 finalColor;

uniform float utime;

mat2 matrot(float a) {
  float c = cos(a), s = sin(a);
  return mat2(c, s, -s, c);
}

void main() {
  vec2 pos = (fpos - 0.5) * matrot(3.14159 * 0.5 * f_rot);
  float h = 1.0 / 18.0;
  // float intensity = (utime + 1.0) * 0.25 * 0.5 + 0.75;
  float intensity = (utime + 1.0) * 0.5 * 0.6  + 0.4;
  // vec4 red = intensity * vec4(1.0, 0.0, 0.0, 1.0);
  // red.a = 1.0;
  vec4 red = vec4(intensity, 0.0, 0.0, 1.0);

  vec2 p0 = vec2(0.0, 0.0);           // D (driver)
  vec2 p1 = vec2(-2.0 * h, -2.0 * h); // A0 (socket 1)
  vec2 p2 = vec2(-2.0 * h, 2.0 * h);  // A1 (socket 2)

  bool hit = (abs(pos.x - p0.x) < h && abs(pos.y - p0.y) < h) ||
             (abs(pos.x - p1.x) < h && abs(pos.y - p1.y) < h) ||
             (abs(pos.x - p2.x) < h && abs(pos.y - p2.y) < h);

  finalColor = hit ? red : vec4(0.0);
}
