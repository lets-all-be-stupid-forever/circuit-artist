
#version 330

in vec2 fragTexCoord;

out vec4 finalColor;
uniform sampler2D texture0;
uniform ivec2 size;
uniform sampler2D layer_above;
uniform ivec2 layer_above_size;

int get_pixel(int x, int y) {
  if (x < 0 || x >= size.x || y < 0 || y >= size.y) {
    return 0;
  }
  vec4 pix = texelFetch(texture0, ivec2(x, y), 0);
  return pix.a > 0.5 ? 1 : 0;
}

// Single pixels
//    U
//  L M R
//    B
#define bM (1 << 0)
#define bU (1 << 1)
#define bR (1 << 2)
#define bB (1 << 3)
#define bL (1 << 4)
// Corners
#define bC1 ((bM | bB))
#define bC2 ((bM | bU))
#define bC3 ((bM | bL))
#define bC4 ((bM | bR))

bool is_layer_via(int code) {
  return ((code == bC1) || /*  */
          (code == bC2) || /*  */
          (code == bC3) || /*  */
          (code == bC4));  /*  */
}

void main() {
  vec2 p = fragTexCoord;
  int w = size.x;
  int h = size.y;
  float dx = 1.0 / w;
  float dy = 1.0 / h;

  int x = int(floor(p.x * size.x));
  int y = int(floor(p.y * size.y));

  int b0 = get_pixel(x, y);
  int b1 = get_pixel(x, y - 1); /* up */
  int b2 = get_pixel(x + 1, y); /* right */
  int b3 = get_pixel(x, y + 1); /* down */
  int b4 = get_pixel(x - 1, y); /* left */
  /* max is 63 */
  int b = b0 | (b1 << 1) | (b2 << 2) | (b3 << 3) | (b4 << 4);
  if (layer_above_size.x > 0) {
    /* Includes the layer via
     * For now only works for half-sized layer
     */
    vec4 pix_up = texelFetch(layer_above, ivec2(x / 2, h / 2 - y / 2 - 1), 0);
    int code = int(round(pix_up.r * 255.0));
    code = code & (63); /* gets last 5 bits */
    if (is_layer_via(code)) {
      b = b | (1 << 6); /* This flag means it has a connection up*/
    }
  }
  float f = float(b) / 255.0;
  finalColor = vec4(f, f, f, 1.0);
}


