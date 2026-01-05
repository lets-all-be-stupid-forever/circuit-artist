#version 330

out vec4 finalColor;
in vec4 v_clr;
uniform int mode; /* 0=normal (reduced color) */

void main()
{
  vec4 c = vec4(v_clr.rgb * 0.5, 1);
  finalColor = c; // vec4(1);
}

