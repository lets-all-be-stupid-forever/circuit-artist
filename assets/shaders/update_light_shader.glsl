#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D tex_l0;
uniform sampler2D tex_l1;

out vec4 finalColor;

void main()
{
  float m = texture(texture0, fragTexCoord).a;
  /* when m is 1 i want to block light from l0, otherwise i just copy */
  vec4 l0 = texture(tex_l0, fragTexCoord);
  vec4 l1 = texture(tex_l1, fragTexCoord);
  vec4 combined =  l0 * (1-m) + (m)*l1;
  finalColor = combined;
}

