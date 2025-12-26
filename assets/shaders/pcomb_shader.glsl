#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
// Input uniform values
uniform sampler2D layer0;
uniform sampler2D layer1;
uniform sampler2D layer2;
uniform sampler2D layer3;
uniform sampler2D layer4;
uniform vec4 colDiffuse;
//uniform int mode;

// Output fragment color
out vec4 finalColor;

uniform vec2 sp; // layer zoom (1 pixel layer = sp pixels screen) (should be an int)
uniform ivec2 off;  // layer offset
uniform ivec2 img_size;  // Layer size
uniform ivec2 tgt_size;  // Dest buffer size

// Step1 :  make a checkerboard transparency on layer1.
// First I need, for each pixel, see the position in the screen.
// c 


float fmin(float a, float b ) {return a < b ? a : b;}

float square(float x){ return x*x;}

float dist2(float aa, float bb){
  return  aa*aa+bb*bb;
}

float dist_to_edge(sampler2D layer) {
  vec2 pos = fragTexCoord;
  float x = pos.x * img_size.x;
  float y = pos.y * img_size.y;
  float xc = floor(x);
  float yc = floor(y);

  /* First corner distances */
  float dpos = 1000;
  float dneg = 1000;
  int ix = int(floor(xc));
  int iy = int(floor(yc));
  {
    int a = (texelFetch(layer, ivec2(ix-1, iy-1), 0).a > 0) ? 1 : 0;
    int b = (texelFetch(layer, ivec2(ix+1, iy-1), 0).a > 0) ? 1 : 0;
    int c = (texelFetch(layer, ivec2(ix+1, iy+1), 0).a > 0) ? 1 : 0;
    int d = (texelFetch(layer, ivec2(ix-1, iy+1), 0).a > 0) ? 1 : 0;
    float da = dist2(xc-x, yc-y);
    float db = dist2(xc+1-x, yc-y);
    float dc = dist2(xc+1-x, yc+1-y);
    float dd = dist2(xc-x, yc+1-y);
    if (a == 1) dpos = fmin(dpos, da);
    if (b == 1) dpos = fmin(dpos, db);
    if (c == 1) dpos = fmin(dpos, dc);
    if (d == 1) dpos = fmin(dpos, dd);
    if (a == 0) dneg = fmin(dneg, da);
    if (b == 0) dneg = fmin(dneg, db);
    if (c == 0) dneg = fmin(dneg, dc);
    if (d == 0) dneg = fmin(dneg, dd);
  }
  /* Lateral distances */
  {
    int a = (texelFetch(layer, ivec2(ix-1, iy), 0).a > 0)? 1 : 0;
    int b = (texelFetch(layer, ivec2(ix, iy-1), 0).a > 0)? 1 : 0;
    int c = (texelFetch(layer, ivec2(ix+1, iy), 0).a > 0)? 1 : 0;
    int d = (texelFetch(layer, ivec2(ix, iy+1), 0).a > 0)? 1 : 0;
    float da = square(x-xc);
    float db = square(y-yc);
    float dc = square(xc+1-x);
    float dd = square(yc+1-y);
    if (a == 1) dpos = fmin(dpos, da);
    if (b == 1) dpos = fmin(dpos, db);
    if (c == 1) dpos = fmin(dpos, dc);
    if (d == 1) dpos = fmin(dpos, dd);
    if (a == 0) dneg = fmin(dneg, da);
    if (b == 0) dneg = fmin(dneg, db);
    if (c == 0) dneg = fmin(dneg, dc);
    if (d == 0) dneg = fmin(dneg, dd);
  }
  {
    if (texelFetch(layer, ivec2(ix, iy), 0).a > 0) {
      dpos = 0;
      dneg = fmin(dneg, 1);
    } else {
      dneg= 0;
      dpos= fmin(dpos, 1);
    }
  }

  return sqrt(dpos) - sqrt(dneg);
}


vec4 togray(vec4 c) {
  vec4 color = c;//vec4(r, g, b, a);
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  vec4 grayscale = vec4(gray, gray, gray, color.a);
  return grayscale;
}

void main()
{
  vec2 pos = fragTexCoord;
  int spi = int(sp);
  int px = int(pos.x * img_size.x * sp.x + off.x);
  int py = int(pos.y * img_size.y * sp.y + off.y);
  int ppx = px / spi;
  int ppy = py / spi;
  px = px /2 ;
  py = py /2 ;

  vec4 img_color1 = texture(layer0, pos);
  vec4 img_color2 = texture(layer1, pos);

   // Distance To pixel = distance to a pixel edge.

  float d1 = dist_to_edge(layer0);
  float d2 = dist_to_edge(layer1);

  float r = 0.3;
  //if (abs(d1) < r) f = 0;
  float f = smoothstep(0, r, d2);
  float f1 = smoothstep(0, r, d1);

  // Now I'll do an outline.
    
  int pat = (px + py) % 4 == 1? 1:0;
  vec4 c;

  vec4 c2 =togray(img_color1);
   c2.rgb = 0.5 * c2.rgb;
#if 1
  if (img_color2.a == 1.0) {
    float k = 1.0;
    //c= k*img_color2+(1-k)*img_color1;
    c= pat*img_color2+(1-pat)*c2;
    c=img_color2;

  } else {
   c= c2;
   //  c= img_color1 ;
  }
#endif
#if 1

  if(d2>0){
//  c.rgb = f*c.rgb;
//  if (c.a == 0 && f < 1) c.a=1-f;
  int k = int(d2*spi);

  int h =  spi/2;

  if (k == h-2) c= 0*img_color2;
  if (k < h-2) c = vec4(1.0, 1.0, 1.0, 0.5);
  }
  if (d2<0) {
    c = vec4(1.0, 1.0, 1.0, 0.5);
  }

#endif
  finalColor = c;

  //finalColor = (img_color2 + img_color1);
  // if (ppx == 255 && ppy == 255) {
  //   finalColor = vec4(0,0,0,1);
  // } else {
  //   finalColor = vec4((px % 256) / 255.0, (py % 256)/255.0, 0.0, 1.0);
  // }
}
