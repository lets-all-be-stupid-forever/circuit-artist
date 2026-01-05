#version 330

in float vert;  // 0.0 = line start, 1.0 = line end
in vec4 pos;
in vec2 dist;
in float wid;

uniform sampler2D pulses;

uniform int pulse_w;
uniform int cur_tick;
uniform int hide_mask;
uniform float slack;
uniform int w;
uniform int h;
uniform mat4 mvp;

out vec2 fragTexCoord;
out vec4 pvalue;
out float v_l;
flat out vec2 v_dist;
flat out vec2 flat_p;
flat out float rc_seg;

vec4 pack_int32(int value) {
  uint uval = uint(value);  // Reinterpret as unsigned
  uvec4 bytes = uvec4(
                      uval & 0xFFu,         // r: lowest byte
                      (uval >> 8) & 0xFFu,  // g: second byte
                      (uval >> 16) & 0xFFu, // b: third byte
                      (uval >> 24) & 0xFFu  // a: highest byte
                     );
  return vec4(bytes) / 255.0;  // Normalize to [0, 1]
}

void main()
{
  v_dist = dist;
    int wid_pack = floatBitsToInt(wid);
    vec4 p = pos;
    int l = (wid_pack >> 1) &7;
    /* if it's hidden, we throw away the vertex by putting it outside clip space */
    if ((hide_mask & (1 << l)) != 0) {
      gl_Position = vec4(1000);
      return;
    }

    int real_wid = wid_pack >> 4;
    float x0, x1, y0, y1, z;
    float t = vert;  // Line parameter: 0.0 to 1.0
    rc_seg = p.w;

    // upper layers are closer to the screen
    float x, y;
    /* Vertical */
    if ((wid_pack & 1) == 1) {
        // Vertical segment: x constant, y varies from p.y to p.z
        x = p.x + 0.5;  // Center of pixel
        y0 = p.y+0.5;
        y1 = p.z + 1.0+0.5;
        y = y0 * (1.0 - t) + t * y1;
        v_l = t;
        z = 2*l + 1;
        flat_p = vec2(y0, y1);
    } else {
        // Horizontal segment: y constant, x varies from p.y to p.z
        x0 = p.y;
        x1 = p.z + 1.0;
        x = x0 * (1.0 - t) + t * x1;
        y = p.x + 0.5;  // Center of pixel
        z = 2*l;
        v_l = t;
        flat_p = vec2(x0, x1);
    }
    // 0 --> H layer 0
    // 1 --> V layer 0
    // 2 --> H layer 1
    // 3 --> V layer 1
    // 4 --> H layer 2
    // 5 --> H layer 2

    z = -z / 20.0;
    float tx = round(x - 0.5);
    float ty = round(y - 0.5);
    int s = 0;

    {
      int px = real_wid % pulse_w;
      int py = real_wid / pulse_w;
      ivec2 coord = ivec2(px, py);
      pvalue = texelFetch(pulses, coord, 0);
    }

    float xx = (x)/w;
    float yy = 1.0 - (y)/h;
    fragTexCoord = vec2(xx, 1.0-yy);
    x = 2.0*x/w-1.0;
    y = (2.0*y/h-1.0);
    gl_Position = mvp * vec4(x, y, z, 1.0);
}
