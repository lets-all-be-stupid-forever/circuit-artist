#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D tex_y;

uniform vec2 f_a;
uniform vec2 f_b;

void main()
{
    vec2 pos = fragTexCoord;
    vec4 vx = texture(texture0, pos);
    vec4 vy = texture(tex_y, pos);
    vec3 v = f_a.x * vx.rgb + f_b.x * vy.rgb;
    //if (v.x < 5.0/255.0) v = vec3(0);
    vec4 vv = vec4(v, 1.0);
    finalColor = vv;
}
