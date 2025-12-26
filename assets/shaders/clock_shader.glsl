#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform vec2 mouse;/* mouse position (pixels) */
uniform vec2 target_size; /* Size of the target */

float distcirc(in vec2 p, in vec2 c, in float r)
{
    return abs(r - length(p - c));
}

float distline(vec2 p, vec2 a, vec2 b)
{
    vec2 ab = b - a;
    vec2 ap = p - a;
    float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    return length(ap - ab * t);
}


// i need this to have an alphachannel in the output
// // i intend to use an optimized version of this shader for a transparent desktop widget experiment
vec4 mixer(vec4 c1, vec4 c2) {
  // please tell me if you think this would boost performance.
  // the time i implemented mix myself it sure did reduce
  // the amount of operations but i'm not sure now
  // if (c2.a <= 0.0) return c1;
  // if (c2.a >= 1.0) return c2;
  return vec4(mix(c1.rgb, c2.rgb, c2.a), c1.a + c2.a);
  // in case you are curious how you could implement mix yourself:
  // return vec4(c2.rgb * c2.a + c1.rgb * (1.0-c2.a), c1.a+c2.a);
}

vec4 line(vec4 c, vec2 p, vec2 a, vec2 b) {
  float d = distline(p, a, b);
  if (d < 2) c = mixer(c, vec4(1,1,1,1));
  return c;
}

void main()
{
  // mouse is relative to the center
  /* -N/2 - N/2 pixel position */
  vec2 pos = (fragTexCoord - 0.5) * target_size;

  //pos = pos + target_size/2.0;
  //pos = pos - 75.0;

  vec4 color = vec4(0);
  float k = length(mouse);
  float R = k*1.1;//50.0;
  //float R = 50.0;

  if (length(pos) < R) {
    color = vec4(0,0,0,1);//mixer(color, vec4(0,0,0,0.8));
  }

  // Color face
  float d = abs(length(pos) - R);  // circle at radius 100
  if (d < 2.0) color = mixer(color, vec4(1,1,1,0.5));

  // Hour marks
  for (int i = 0; i < 12; i++) {
    float angle = float(i) * 3.14159 * 2.0 / 12.0;
    vec2 markDir = vec2(sin(angle), cos(angle));
    vec2 markPos = markDir * R * 0.9;  // position at radius 90
    float d = length(pos - markPos);
    if (d < 2.5) color = mixer(color, vec4(1,1,1,0.5));
  }

  // Minute marks (smaller)
  for (int i = 0; i < 60; i++) {
    if (i % 5 == 0) continue;  // skip hour positions
    float angle = float(i) * 3.14159 * 2.0 / 60.0;
    vec2 markDir = vec2(sin(angle), cos(angle));
    vec2 markPos = markDir * R * 0.9;
    float d = length(pos - markPos);
    if (d < 1.5) color = mixer(color, vec4(0.5,0.5,0.5,0.5));
  }

  // Center dot
  if (length(pos) < 5.0) color = mixer(color, vec4(1,1,1,1));

  vec2 dir = normalize(mouse);
  //dir = dir / (length(dir) + 1e-6);

  color = line(color, pos, vec2(0), dir * R * 0.8 );

  // color = line(color, pos, vec2(0), mouse );

#if 0
  {
    float d = length(pos - mouse);
    if (d < 5) color = mixer(color, vec4(1));
  }

  {
    float d = distline(pos, vec2(0), mouse);
    if (d < 2)color = mixer(color, vec4(1,1,0,1));
  }
#endif

  if (color.a > 1) color.a = 1;

  float dmouse = length(pos - mouse);
float dpos = length(pos);
float amouse = 0.7 * (1.0-smoothstep(30, 90, dmouse));
float apos = 0.7 * (1.0-smoothstep(30, 90, dpos));

  color.a = max(amouse, apos) * color.a;

  //color.a = 0.8 * color.a;
  finalColor = color;
}

