/*
 * Just a constant shader to show  a depth map
 */
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec2 depth;

out vec4 finalColor;
void main() {
  vec2 pos = fragTexCoord;
  vec4 c = texture(texture0, pos);
  if (c.a > 0) {
    c.rgb = vec3(depth.x);
    //c.rgb = depth.x * c.rgb;
  }
  finalColor = c;
}

