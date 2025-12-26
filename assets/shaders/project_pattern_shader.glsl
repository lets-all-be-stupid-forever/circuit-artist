#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform int pattern;
uniform int pad;

// Output fragment color
out vec4 finalColor;

uniform vec2 sp; // zoom
uniform ivec2 img_size; // image size in image pixels

vec4 get_pixel(int x, int y) {
  if (x < 0 || x >= img_size.x || y < 0 || y >= img_size.y) {
    return vec4(0.0, 0.0, 0.0, 0.0);
  }
  return texelFetch(texture0, ivec2(x, y), 0);
}

vec4 postproc(int xx, int yy, vec4 c) {
  if (pattern == 0) {
    if (((xx+yy))%4 == 1) return c;
    else {
      return vec4(0,0,0,0);
    }
  } else if (pattern == 1) {
    if (((xx-yy))%8 == 0) return c;
    else {
      return vec4(0,0,0,0);
    }
  } else if (pattern == 2){
    if (c.a > 0.5) c.a = 0.5;
    return c;
  }

}


int get_dist(int x, int y, int ee, int k) {
  int e = ee + k;
  int dx = e-x;
  int dy = e-y;
  //int ds = 2*e - k - (x+y);
  int ds = dx;
  ds = ds < dx? ds: dx;
  ds = ds < dy? ds: dy;
  return ds;
}

int f_corner(int x, int y, int e) {
  if (x < e) return 1;
  if (y < e) return 1;
  if (x == e) return 0;
  if (y == e) return 0;
  return -1;
}

// horizontal edge
int f_edge(int x, int e) {
  if (x < e) return 1;
  if (x == e) return 0;
  return -1;
}


//    NEG = interior
//    ------
//    POS = negative
//   . - .
//   | c |
//   . - .
// ignoring color for now
// 1. dist to each center.
// 1 = -1
// 0 = +1
vec4 sample_zoom_in_pattern(){
  vec2 pos = fragTexCoord;
  vec4 img_color;
  // Need an even number of samples to make it work here.
  int p = int(round(sp.x));

  int w = img_size.x;
  int h = img_size.y;

  float fx = pos.x * img_size.x - 0.5;
  float fy = pos.y * img_size.y - 0.5;

  int ix = int(floor(fx));
  int iy = int(floor(fy));
  int px = int(floor((fx- float(ix)) * float(p)));
  int py = int(floor((fy- float(iy)) * float(p)));

  vec4 c00 = get_pixel(ix+0, iy+0);
  vec4 c01 = get_pixel(ix+1, iy+0);
  vec4 c10 = get_pixel(ix+0, iy+1);
  vec4 c11 = get_pixel(ix+1, iy+1);

  int xx = ix*p + px;
  int yy = iy*p + py;

  // Inverted pixel position
  int qx = p-1-px;
  int qy = p-1-py;

  int e =  p/2;

  int b00 = c00.a > 0.5 ?1:0;
  int b01 = c01.a > 0.5 ?1:0;
  int b10 = c10.a > 0.5 ?1:0;
  int b11 = c11.a > 0.5 ?1:0;
  int b = b00 | (b01 << 1) | (b10 << 2) | (b11 << 3);
   return postproc(xx, yy, vec4(1));
}


void main()
{
  vec4 img_color = sample_zoom_in_pattern();
  img_color.rgb = vec3(1);
  finalColor = img_color * colDiffuse *fragColor;
}
