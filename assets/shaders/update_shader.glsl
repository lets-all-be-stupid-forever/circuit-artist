#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D sel;
uniform sampler2D tool;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

uniform ivec2 img_size; // image size in image pixels
uniform ivec2 sel_size; // selection size in image pixels
uniform ivec2 sel_off; // selection offset in image pixels
uniform ivec2 tool_size; 
uniform ivec2 tool_off; 

vec4 sample(vec2 pos) {
  if (pos.x < 0.0 || pos.y < 0.0 || pos.x >= 1.0 || pos.y >= 1.0) {
    return vec4(0.0, 0.0, 0.0, 0.0);
  }
  vec4 img_color = texture(texture0, pos);
  float fx = float(img_size.x);
  float fy = float(img_size.y);
  // pixel coordinate image 
  int px = int(round(pos.x * fx - 0.5));
  int py = int(round((1.0 - pos.y) * fy - 0.5));
  if (sel_size.x > 0.1) {
    int sx = px - sel_off.x;
    int sy = py - sel_off.y;
    if (sx >= 0 && sy >= 0 && sx < sel_size.x && sy < sel_size.y) {
      float fsx = float(sel_size.x );
      float fsy = float(sel_size.y );
      // selection pixel is present
      float sel_tex_x = (float(sx) + 0.5)/ fsx;
      float sel_tex_y = (float(sy) + 0.5)/ fsy;
      vec4 sel_color = texture(sel, vec2(sel_tex_x, 1.0 - sel_tex_y));
      if (sel_color.a > 0.5) {
        img_color = sel_color;
      }
    }
  }
  if (tool_size.x > 0.1) {
    int sx = px - tool_off.x;
    int sy = py - tool_off.y;
    if (sx >= 0 && sy >= 0 && sx < tool_size.x && sy < tool_size.y) {
      float fsx = float(tool_size.x );
      float fsy = float(tool_size.y );
      // toolection pixel is present
      float tool_tex_x = (float(sx) + 0.5)/ fsx;
      float tool_tex_y = (float(sy) + 0.5)/ fsy;
      vec4 tool_color = texture(tool, vec2(tool_tex_x, 1.0 - tool_tex_y));
      if (tool_color.a > 0.5) {
        img_color = tool_color;
      }
    }
  }
  return img_color;
}

void main()
{
  vec4 img_color = sample(fragTexCoord);
  finalColor =  img_color * colDiffuse *fragColor;
}
