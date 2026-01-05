#version 330

in float vert;  // 0.0 = line start, 1.0 = line end
in vec4 clr;
in vec2 pos;

uniform int w;
uniform int h;
out vec4 v_clr;

void main() {
  v_clr = clr;
  float x = pos.x + vert;
  float y = pos.y + 0.5;
  x = (x / w);
  y = (y / h);
  x = 2.0 * x- 1.0;
  y = -2.0 * y + 1.0;
  gl_Position = vec4(x, y, 0, 1.0);
}
