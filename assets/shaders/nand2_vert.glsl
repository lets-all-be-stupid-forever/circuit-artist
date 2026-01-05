#version 330
layout(location = 0) in vec2 pos;
uniform vec2 screen_size;
void main() {
  vec2 ndc = (vec2(pos) + 0.5) / screen_size * 2.0 - 1.0;
  gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);  // flip Y for texture coords
  gl_PointSize = 1.0;
}

