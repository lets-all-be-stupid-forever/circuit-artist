#version 330
out vec4 finalColor;
in vec2 fragTexCoord;
uniform ivec2 size;
uniform int dir;
uniform sampler2D texture0;

void main()
{
  vec2 p = fragTexCoord;
  // vec4 src = texture(texture0, pos);
  // src.b = 1.0;

  int w = size.x;
  int h = size.y;
  float dx = 1.0 / w;
  float dy = 1.0 / h;

  vec3 acc = vec3(0);
  // x
  if (dir == 1) {
    acc += texture(texture0, vec2(p.x - 4.0 * dx, p.y)).rgb* 0.05;
    acc += texture(texture0, vec2(p.x - 3.0 * dx, p.y)).rgb* 0.09;
    acc += texture(texture0, vec2(p.x - 2.0 * dx, p.y)).rgb* 0.12;
    acc += texture(texture0, vec2(p.x - 1.0 * dx, p.y)).rgb* 0.15;
    acc += texture(texture0, vec2(p.x - 0.0 * dx, p.y)).rgb* 0.18;
    acc += texture(texture0, vec2(p.x + 1.0 * dx, p.y)).rgb* 0.15;
    acc += texture(texture0, vec2(p.x + 2.0 * dx, p.y)).rgb* 0.12;
    acc += texture(texture0, vec2(p.x + 3.0 * dx, p.y)).rgb* 0.09;
    acc += texture(texture0, vec2(p.x + 4.0 * dx, p.y)).rgb* 0.05;
  } else {
    acc += texture(texture0, vec2(p.x, p.y - 4.0 * dy)).rgb* 0.05;
    acc += texture(texture0, vec2(p.x, p.y - 3.0 * dy)).rgb* 0.09;
    acc += texture(texture0, vec2(p.x, p.y - 2.0 * dy)).rgb* 0.12;
    acc += texture(texture0, vec2(p.x, p.y - 1.0 * dy)).rgb* 0.15;
    acc += texture(texture0, vec2(p.x, p.y - 0.0 * dy)).rgb* 0.18;
    acc += texture(texture0, vec2(p.x, p.y + 1.0 * dy)).rgb* 0.15;
    acc += texture(texture0, vec2(p.x, p.y + 2.0 * dy)).rgb* 0.12;
    acc += texture(texture0, vec2(p.x, p.y + 3.0 * dy)).rgb* 0.09;
    acc += texture(texture0, vec2(p.x, p.y + 4.0 * dy)).rgb* 0.05;
  }
  finalColor = vec4(acc, 1.0);
}
