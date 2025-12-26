
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform int mode;

// Output fragment color
out vec4 finalColor;

uniform vec2 sp; // zoom
uniform ivec2 img_size; // image size in image pixels

vec4 msample(vec2 pos, int r) {
  if (mode == 0) {
    float kx = 1.0 / float(img_size.x);
    float ky = 1.0 / float(img_size.y);
    vec4 ss = vec4(0.0,0.0,0.0,0.0);
    float cnt = 0.0;
    for (int y = -r; y < r; y++) {
      for (int x = -r; x < r; x++) {
        vec4 s = texture(texture0, vec2(pos.x + (x+0.5)*kx , pos.y +(y+0.5)*ky));
        // it averages the pixels even if they are black (ie a=0)
        // It's ok for drawing mode, but for sim mode it throws away on/off info
        // if it's done with alphas, so need to use a=1 on simu.
        if (true) {
          s.a = 1.0;
          ss = ss + s;
          cnt += 1.0;
        }
      }
    }
    return ss / cnt;
  } else if (mode == 1) {
    float kx = 1.0 / float(img_size.x);
    float ky = 1.0 / float(img_size.y);
    vec4 ss = vec4(0.0,0.0,0.0,0.0);
    float cnt = 0.0;
    for (int y = -r; y < r; y++) {
      for (int x = -r; x < r; x++) {
        vec4 s = texture(texture0, vec2(pos.x + (x+0.5)*kx , pos.y +(y+0.5)*ky));
        // it averages the pixels even if they are black (ie a=0)
        // It's ok for drawing mode, but for sim mode it throws away on/off info
        // if it's done with alphas, so need to use a=1 on simu.
        if (s.r + s.g + s.b> 0.1) {
          s.a = 1.0;
          ss = ss + s;
          cnt += 1.0;
        }
      }
    }
    return ss / cnt;

  }
}

vec4 get_pixel(int x, int y) {
  if (x < 0 || x >= img_size.x || y < 0 || y >= img_size.y) {
    return vec4(0.0, 0.0, 0.0, 0.0);
  }
  return texelFetch(texture0, ivec2(x, y), 0);
}

//   0     h    p
// 0 A ---abbb- B
//   |    abbb  |
//   aaaaaabbbbbb
// h ccccccdddddd
//   c    cd    |
// p C ---cd----D
//


vec4 postproc(int xx, int yy, vec4 c) {
  if (mode %8 == 0) {
    if (((xx+yy))%4 == 1) return c;
    else {
      return vec4(0,0,0,0);
    }
  } else {
    if (((xx-yy))%8 == 0) return c;
    else {
      return vec4(0,0,0,0);
    }
  }

}


vec4 sample_zoom_in(){
  vec2 pos = fragTexCoord;
  vec4 img_color;
  int p = int(round(sp.x));
  int w = img_size.x;
  int h = img_size.y;
  /* float reliable until 16,777,216
   * Image dim goes until 8k, 32k max.
   * zoom goes until 64, 
   * max = 32k*64 = 2M, so it should be ok.
   */
  int ix = int(floor(pos.x * img_size.x));
  int iy = int(floor(pos.y * img_size.y));
  int px = int(floor((pos.x * img_size.x - float(ix)) * float(p)));
  int py = int(floor((pos.y * img_size.y - float(iy)) * float(p)));
  int xx = ix*p+px;
  int yy = iy*p+py;
  int hp = (p-1)/2;

  // vec4 c = texture(texture0, pos);
  vec4 c = get_pixel(ix, iy);

  int bot = get_pixel(ix, iy+1).a > 0.5? 1: 0;
  int up = get_pixel(ix, iy-1).a > 0.5? 1: 0;
  int left = get_pixel(ix-1, iy).a > 0.5? 1: 0;
  int right= get_pixel(ix+1, iy).a > 0.5? 1: 0;
  if (py == hp && px >= hp && right == 1) return c; 
  if (py == hp && px < hp && left == 1) return c; 
  if (px == hp && py >= hp && bot== 1) return c; 
  if (px == hp && py < hp && up== 1) return c; 

#if 1
  if (true){
    if (px <= 0 && left==0) return c;
    if (px >= p-1 && right==0) return c;
    if (py <= 0 && up==0) return c;
    if (py >= p-1 && bot==0) return c;
  }

 //  else {
 //    int t1 = 1;//p/8;
 //    int t2 = 2;//t1+1;
 //    if (px <= t1 && left==0) return vec4(0);
 //    if (px >= p-1-t1 && right==0) return vec4(0);
 //    if (py <= t1 && up==0) return vec4(0);
 //    if (py >= p-1-t1 && bot==0) return vec4(0);

 //  if (px <= t2 && left==0) return c;
 //  if (px >= p-1-t2 && right==0) return c;
 //  if (py <= t2 && up==0) return c;
 //  if (py >= p-1-t2 && bot==0) return c;
 //  }
#endif

#if 0
  if (px == hp-1 && left == 0) return c;
  if (px == hp && right== 0) return c;
  if (py == hp-1 && up== 0) return c;
  if (py == hp && bot== 0) return c;
#endif

  //if (px == 0 || px == p-1 || py==0 || py ==p-1) {
  //  return c;
  //}
  return postproc(xx, yy, c);

}


void main()
{
  // Texel color fetching from texture sampler
  vec2 pos = fragTexCoord;
  vec4 img_color;
  if (sp.x > 4.0 -1e-2) {
    /* Here we start handling zoom-in */
    img_color = sample_zoom_in();
img_color .rgb = vec3(1);
  } else if (sp.x > 1.0 - 1e-2) {
    img_color = texture(texture0, pos);
  } else if (sp.x > 0.5 - 1e-2) {
    img_color = msample(pos, 1);
  } else if (sp.x > 0.25 - 1e-2) {
    img_color = msample(pos, 2);
  } else {
    img_color = msample(pos, 4);
  }
  finalColor = img_color * colDiffuse *fragColor;
}
