#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D dst_tex;

uniform ivec2 roi_size;
uniform ivec2 src_size;
uniform ivec2 src_off;
uniform ivec2 dst_off;
uniform ivec2 dst_size;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main()
{
    vec4 src = texture(texture0, fragTexCoord);
    float src_dx = float(src_size.x);
    float src_dy = float(src_size.y);
    float dst_dx = float(dst_size.x);
    float dst_dy = float(dst_size.y);

    // pixel coordinate image in source
    int src_x = int(round(fragTexCoord.x * src_dx - 0.5));
    int src_y = int(round((1.0 - fragTexCoord.y) * src_dy - 0.5));

    // pixel coordinate image in dest
    int dst_x = src_x - src_off.x + dst_off.x;
    int dst_y = src_y - src_off.y + dst_off.y;

    float dst_tx = (float(dst_x) + 0.5) / dst_dx;
    float dst_ty = 1.0 - ((float(dst_y) + 0.5) / dst_dy);
    vec2 dst_pos = vec2(dst_tx, dst_ty);
    vec4 dst = texture(dst_tex, dst_pos);

    vec4 c = dst;
    if (src.r == 0.0 && src.g == 0.0 && src.b == 0.0 && src.a == 1.0) {
        c = vec4(0.0, 0.0, 0.0, 0.0);
    } else if (src.a != 0.0) {
        c = src;
    }
    finalColor = c*colDiffuse;
}

