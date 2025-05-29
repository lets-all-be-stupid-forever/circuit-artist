#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D sel;
uniform sampler2D tool;
uniform vec4 colDiffuse;

uniform int mode;

// Simulation
uniform int simu_state;
uniform float unchanged_alpha;
uniform vec4 bg_color;
uniform vec4 bugged_color;
uniform vec4 undefined_color;
uniform sampler2D comp_x;
uniform sampler2D comp_y;
uniform sampler2D state_buf;

uniform sampler2D prev_state_buf;
uniform vec4 prev_state_f;

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
  if (mode == 0) {
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
  } else if (mode == 1) {
    // Simulation
    vec4 src = texture(texture0, pos);
    vec4 tx = texture(comp_x, pos);
    vec4 ty = texture(comp_y, pos);

    vec4 texelColor;
    texelColor = src;
    vec4 ts = texture(state_buf, vec2(tx.r, ty.r));
    vec4 prev_ts = texture(prev_state_buf, vec2(tx.r, ty.r));

    texelColor = src;
    float s = ts.r;
    float sprev = prev_ts.r;
    float f = prev_state_f.r;
    if (s <= 0.05) {
      // state=Background
      texelColor = bg_color;
    } else if (s < 0.15) {
      // state=bugged texelColor = ;
      texelColor = bugged_color; // vec4(1.0, 0.0, 0.0, 1.0);
    } else if (s < 0.25) {
      // state=undefined
      // undefined = -1
      texelColor = undefined_color;
      // When the simulation is in "bugged" state, display undefined wires a
      // bit darker
      if (simu_state == 1) {
        texelColor.rgb = 0.1 * texelColor.rgb;
      }
    } else if (s < 0.35) {
      // Wire is off
      vec3 off = ( 75.0 / 255.0) * texelColor.rgb;
      vec3 on = texelColor.rgb;
      if (sprev != s) {
        // vec3 r = on * f + (1.0 -f) * off;
        vec3 r = off;
        texelColor.rgb = r;
      } else {
        texelColor.rgb = off;
        texelColor.a = unchanged_alpha;
      }

    } else if (s < 0.45)  {
      // Wire is on
      // texelColor.a = 1.0;

      vec3 off = ( 75.0 / 255.0) * texelColor.rgb;
      vec3 on = texelColor.rgb;
      if (sprev != s) {
        // vec3 r = off * f + (1.0 -f) * on;
        vec3 r = on;
        texelColor.rgb = r;
      } else {
        texelColor.a = unchanged_alpha;
        texelColor.rgb = on;
      }

    } else if (s < 0.55)  {
      // default nand color
      if (simu_state == 0)  {
        texelColor.a = (175.0/255.0);
      }
      else {
        texelColor.a = 0.1;
      }
    }
    return texelColor;
  }
}

void main()
{
  vec4 img_color = sample(fragTexCoord);
  finalColor =  img_color * colDiffuse *fragColor;
}
