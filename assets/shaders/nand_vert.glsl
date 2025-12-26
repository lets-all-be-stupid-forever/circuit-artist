#version 330

in vec3 vert;
in vec4 pos;

in vec4 c1;
in vec4 c2;
in vec4 c3;

uniform int tgt_h;
uniform int tgt_w;
uniform mat4 mvp;

in int bugged;
out float f_bugged;
out vec2 txy;

out vec4 fc1;
out vec4 fc2;
out vec4 fc3;

void main()
{
  if (bugged == 0) {
  f_bugged = -1.0;
  }else {
  f_bugged = 1.0;
  }
  fc1 = c1;
  fc2 = c2;
  fc3 = c3;
    vec4 p = pos; 
    float x0 = p.x;
    float y0 = p.y;
    float x1 = p.z;
    float y1 = p.w;
    float vx = vert.x;
    float vy = vert.y;

    //txy = vec2(vx * (x1-x0), vy * (y1-y0));
    txy = vert.xy;
    //txy = vec2(vx, vy);
    // this is in image pixels 
    float x = x0 * (1.0 -vx) + vx * x1;
    float y = y0 * (1.0 -vy) + vy * y1;
    //x = 2.0*x/w-1.0;
    //y = 2.0*y/h-1.0;
    // gl_Position = mvp * vec4(x, y, 0.0, 1.0);
    gl_Position = mvp * vec4(x, y, 0.0, 1.0);
    // gl_Position = vec4(vx*0.5, vy*0.5, 0.0, 1.0);
    // gl_Position = vec4(vx, vy, 0.0, 1.0);
}
