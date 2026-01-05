#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
uniform vec2 phase;
uniform vec4 cnand1; // Socket 1
uniform vec4 cnand2; // Socket 2
uniform vec4 cnand3; // Driver
out vec4 finalColor;


mat2 matrot(float a) {
 float c = cos(a), s = sin(a);
 return mat2(c,s,-s,c);
}

float sdbox2d(vec2 p, vec2 b)
{
    vec2 d = abs(p) - b;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

//
//   A0 0 B0 0
//      0 0  C 0 D
//   A1 0 B1 0
//
// Coordinates: (center)
// C = (0,0)
// D = (4h,0)
// B0 = (-2h,-2h)
// B1 = (-2h,2h)
// A0 = (-6h,-2h)



vec4 pixel(vec4 color, vec2 p, float h, vec2 c, vec4 clr) {
    float d = sdbox2d(p - c, vec2(h));
    if (d > 0) return color;
    return color + clr;
}

float gfunc(float x) {
  float r = 0.1;
  return smoothstep(0, 0.5-r, x) - smoothstep(0.5+r, 1, x);
}


// f goes from 0 to 1
vec4 rayphase_old(vec4 color, vec2 p, float h, vec2 c, float f) {
  // 1.0 --> start
  // 0.0 --> end

  if (f < 0  || f > 1.0) return color;
  float d = sdbox2d(p - c, vec2(h));
  if (d > 0) return color;

    float x0 = c.x - h;
    float x1 = c.x + h;

    float dy = (p.y - c.y)/h; /* -1 (top)--> +1 (bottom)*/
    float dx = (p.x -  x0)/h;

    if (abs (dy) < 0.3) color = color + vec4(1) ;
    return color;
}

vec4 rayphase(vec4 color, vec2 p, float h, vec2 c, vec4 pclr, float f) {
  // 1.0 --> start
  // 0.0 --> end
  if (f < 0  || f > 1.0) return color;
  float d = sdbox2d(p - c, vec2(h));
  if (d > 0) return color;

  float x0 = c.x - h;
  float x1 = c.x + h;

  float xx = (p.x - x0)/(2*h);
  float v = gfunc(xx + (1 - 2*f)*0.75);
  color = color + 0.3 * vec4(v*pclr.rgb, 1);
  return color;
}

vec4 tray(vec4 color, vec2 p, float h) {
  float ct = phase.x;
  float mt = 0.5; /* time for this phase*/
  float t = 1.0 - ct;
  float f1 = t / mt;

  float f2 = (t - (1.0-mt))/ mt;
  color = rayphase(color, p, h, vec2(-4*h, 2*h), cnand1, f1);
  color = rayphase(color, p, h, vec2(-4*h, -2*h), cnand2, f1);
  color = rayphase(color, p, h, vec2(2*h, 0), cnand3, f2);
  return color;
}

void main()
{
  float nrot = phase.y;
    vec2 pos = (fragTexCoord - 0.5) * matrot(3.14159 * 0.5 * nrot );
    float r2 = length(pos);
    float p = phase.x;
    float rx = pos.x;
    float ry = pos.y;
    float r = length(pos);
    vec4 color = vec4(0);
    float h = 1.0/18.0;
    float f = gfunc(phase.x);
    color = pixel(color, pos, h, vec2(0, 0), f * cnand3); // D
    color = pixel(color, pos, h, vec2(-2*h, -2*h), f * cnand1); // A0
    color = pixel(color, pos, h, vec2(-2*h, 2*h), f * cnand2); // A1
    color = tray(color, pos, h); // A1
    finalColor = color;
}

