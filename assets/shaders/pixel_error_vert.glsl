#version 330

in vec3 vert;
in vec2 pos;

out vec2 f_vert;

uniform mat4 mvp;
uniform float utime;
uniform float zoom;

void main()
{
    f_vert = vert.xy;
    vec2 p = pos; 
    float x0 = p.x;
    float y0 = p.y;
    float x1 = x0 + 1.0;
    float y1 = y0 + 1.0;
    float t = 0.5 * (utime + 1);
    t = 5.0*t / zoom;

    x0 = x0 - 1 * t;
    y0 = y0 - 1 * t;
    x1 = x1 + t;
    y1 = y1 + t;
    float vx = vert.x;
    float vy = vert.y;
    float x = x0 * (1.0 -vx) + vx * x1;
    float y = y0 * (1.0 -vy) + vy * y1;
    gl_Position = mvp * vec4(x, y, 0.0, 1.0);
}


