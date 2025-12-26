#version 330

out vec4 finalColor;
in vec2 f_vert;

void main()
{
  float x = abs(f_vert.x - 0.5);
  float y = abs(f_vert.y - 0.5);
  if (x > 0.4 || y > 0.4) {
  finalColor = vec4(1.0, 0.0, 0.0, 1.0);
  } else {
  finalColor = vec4(0.0, 0.0, 0.0, 0.0);
  }
}

