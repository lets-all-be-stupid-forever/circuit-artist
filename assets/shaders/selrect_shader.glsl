
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;


// Input uniform values
//uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 rsize;
uniform vec2 rect_pos;
uniform int pattern_shift;
uniform int pattern_width;

// Output fragment color
out vec4 finalColor;

void main()
{
  // fragTexCoord
  //int fx = int(round(fragTexCoord.x * rect_size.x - 0.5));
  int fx = int(round(fragTexCoord.x * rsize.x - 0.5));
  int fy = int(round(fragTexCoord.y * rsize.y - 0.5));
  int ix = int(round(rect_pos.x));
  int iy = int(round(rect_pos.y));

  int p = ix+fx+iy+fy+pattern_shift;
  p = p % (2*pattern_width);
  if (p < pattern_width)  {
    finalColor = vec4(1.0,1.0,1.0,1.0);
  } else {
    finalColor = vec4(0.0,0.0,0.0,1.0);
  }
  // vec4 texelColor = texture(texture0, fragTexCoord);
  // finalColor = texelColor*colDiffuse*fragColor;
  //finalColor = vec4(1.0,0.0,0.0,1.0);
}
