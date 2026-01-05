#version 330

// Per-vertex attributes
in vec3 vert;  // x,y = quad corner (0-1), z unused

// Per-instance attributes (divisor = 1)
in vec2 pos;      // NAND center position in pixels
in float rot;     // rotation

// Outputs to fragment shader
out vec2 fpos;          // Normalized position within quad (0-1)
flat out float f_rot;

uniform int w;
uniform int h;

void main() {
  float sw = 9.0;  // Quad width in pixels
  float sh = 9.0;  // Quad height in pixels

  // Pass to fragment shader
  fpos = vert.xy;
  f_rot = rot;

  // Calculate pixel position
  // pos is the center, offset by -5 to get top-left corner (matching nandact3 logic)
  float x = (pos.x - 5.0) + vert.x * sw;
  float y = (pos.y - 5.0) + vert.y * sh;

  // Convert to normalized device coordinates
  x = (x / float(w));
  y = (y / float(h));
  x = 2.0 * x - 1.0;
  y = -2.0 * y + 1.0;

  gl_Position = vec4(x, y, 0.0, 1.0);
}
