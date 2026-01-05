#version 330
out vec4 finalColor;
uniform sampler2D dmap;

in vec2 fragTexCoord;
in vec4 pvalue;
in float ori;

float unpack_float32(vec4 rgba) {
  uvec4 bytes = uvec4(rgba * 255.0 + 0.5);
  uint uval = (bytes.a << 24) | (bytes.b << 16) |
              (bytes.g << 8) | bytes.r;
  return uintBitsToFloat(uval);
}

void main()
{
  /* I only update if dmap is the same orientation */
  float d = unpack_float32(texture(dmap, fragTexCoord));
  if ((ori > 0.5 && d <= 0) || (ori < 0.5 && d >= 0)) {
    finalColor = pvalue;
  } else {
    discard;
  }
  // finalColor = vec4(0);
  //finalColor = vec4(0,1,1,0.5);
}
