#version 330
in vec2 fragTexCoord;

layout(location = 0) out vec4 out_circ;
layout(location = 1) out vec4 out_light;

uniform sampler2D texture0;
uniform sampler2D pmap;

uniform sampler2D prev_circ;
uniform sampler2D prev_light;

uniform vec4 colDiffuse;
uniform int tick;
uniform float slack;
uniform int error_mode;
uniform float glow_dt;
uniform float utime;
uniform float ema_factor;

uniform int tickmod64;
uniform int tickgap64;

vec4 g_c_on;
vec4 g_c_off;

float unpack_float32(vec4 rgba) {
  uvec4 bytes = uvec4(rgba * 255.0 + 0.5);
  uint uval = (bytes.a << 24) | (bytes.b << 16) |
              (bytes.g << 8) | bytes.r;
  return uintBitsToFloat(uval);
}

int unpack_int32(vec4 rgba) {
  uvec4 bytes = uvec4(rgba * 255.0 + 0.5);  // Denormalize
  uint uval = (bytes.a << 24) | (bytes.b << 16) |
    (bytes.g << 8) | bytes.r;      // Reassemble bytes
  return int(uval);
}

float luminance(vec3 c) {
  return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec4 grayscale_darker(vec4 color) {
  float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
  gray *= 0.3;  // Reduce intensity to 30%
  return vec4(vec3(gray), 1.0);  // Fully opaque
}

vec4 getlight(float f, vec4 c, int v, int v1, vec4 c_on, vec4 c_off) {
  vec4 fc = vec4(0.0, 0.0, 0.0, 1.0);
  float r = 2.0;
  if (f > 0 && f < glow_dt) {
    float k = 1.0 - smoothstep(glow_dt*.5, glow_dt, f);
    // k is the distance to the tip. 1.0 is on the tip.
    vec4 cc;
    if (v1 == 0) {
      cc = (1.0-k) * c_on + k* c_off;
    } else {
      cc = (1.0-k) * c_off + k* c_on;
    }
    fc = k * cc;
  } else {
    if (v == 1) {
      // float t = (1 +sin(2*3.1415*utime));
      float t = 0;
      fc.rgb = c.rgb * 0.15 + 0.05;
     }
  }
  fc.a = 1.0;
  return fc;
}

vec4 getcirc(float f, int v, vec4 c_on, vec4 c_off) {
  vec4 fc = c_on;
  vec4 c_und = vec4(1.0, 0.0, 1.0, 1.0);
  if (error_mode == 1) {
    // Error mode: red for v=2 (errors), grayscale_darker for v=0 (off), background stays background
    if (c_on.a == 0.0) {
      // Background pixel - keep it transparent
      fc = c_on;
    } else if (v == 2 || v == 3) {
      // Error state - animated red
      fc = ((utime+1.0)*0.5*0.6 + 0.4) * vec4(1.0, 0.0, 0.0, 1.0);
      fc.a = 1.0;
    } else if (v == 0) {
      // Off state - grayscale darker version of original color
      fc = grayscale_darker(c_on);
    } else {
      // v == 1, keep original wire color
      fc = c_on;
    }
  } else {
    // Normal mode - original behavior
    if (v == 0) fc = c_off;
    if (v == 1) fc = c_on;
    if (v == 2) fc = c_und;
    fc.a = c_on.a;
  }
  return fc;
}
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void get_color_on_off_(vec4 c) {
  if (c.a < 0.5) {
    g_c_on = vec4(0,0,0,255)/255.0;
    g_c_off = vec4(0,0,0,255)/255.0;
  } else {
    g_c_on = vec4(191, 162, 82 , 255.0)/255.0;
    g_c_off = vec4(165, 19, 19 , 255.0)/255.0;
  }
}

float random(vec2 st) {
    return fract(sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453);
}


void get_color_on_off(vec4 c) {
  g_c_on = c;
  g_c_off = vec4(c.rgb*0.33, 1);
 //  float noise = random(fragTexCoord + fract(utime * 7.0));
 //  //if (noise < 0.3) g_c_on  = g_c_off;
 //  g_c_on  = mix(g_c_off, g_c_on, mix(0.5, 1.0, noise));
}// 

void get_color_on_off__(vec4 c) {
  vec3 k = rgb2hsv(c.rgb);
float h = k.r;
float s = k.g;
float v = k.b;
  vec3 on = vec3(
      h,
      s*0.5, 
      v* 1.5
  );
  vec3 off = vec3(
      h,
      s * 1.5,
      v * 0.5
  );


  g_c_on = vec4(hsv2rgb(on), 1);
  g_c_off = vec4(hsv2rgb(off), 1);

}


vec4 get_off_color_o(vec4 c_on) {
  vec4 k = c_on;
  k.rgb = 0.4 * c_on.rgb;
  return k;

  float lum = luminance(c_on.rgb);
  vec3 gray = 0.6 * vec3(lum);
  float satmix = 0.0;
  vec3 c = mix(gray, c_on.rgb, satmix);
 return vec4(c, 1.0);
}

vec3 rgb2hsl(vec3 c) {
    float maxC = max(c.r, max(c.g, c.b));
    float minC = min(c.r, min(c.g, c.b));
    float l = (maxC + minC) * 0.5;
    
    if (maxC == minC) return vec3(0.0, 0.0, l);
    
    float d = maxC - minC;
    float s = l > 0.5 ? d / (2.0 - maxC - minC) : d / (maxC + minC);
    
    float h;
    if (maxC == c.r) {
        h = (c.g - c.b) / d + (c.g < c.b ? 6.0 : 0.0);
    } else if (maxC == c.g) {
        h = (c.b - c.r) / d + 2.0;
    } else {
        h = (c.r - c.g) / d + 4.0;
    }
    
    return vec3(h / 6.0, s, l);
}

vec3 hsl2rgb(vec3 hsl) {
    if (hsl.y == 0.0) return vec3(hsl.z);
    
    float q = hsl.z < 0.5 ? hsl.z * (1.0 + hsl.y) : hsl.z + hsl.y - hsl.z * hsl.y;
    float p = 2.0 * hsl.z - q;
    
    vec3 t = fract(vec3(hsl.x) + vec3(1.0/3.0, 0.0, -1.0/3.0));
    
    vec3 rgb;
    for (int i = 0; i < 3; i++) {
        float tc = t[i];
        if (tc < 1.0/6.0) {
            rgb[i] = p + (q - p) * 6.0 * tc;
        } else if (tc < 0.5) {
            rgb[i] = q;
        } else if (tc < 2.0/3.0) {
            rgb[i] = p + (q - p) * (2.0/3.0 - tc) * 6.0;
        } else {
            rgb[i] = p;
        }
    }
    return rgb;
}

/* returns a - b on TICKMOD64
 * Idea: a and b are mod N
 *  but values higher than a are mostly considered to come 
 * a is the tick 
 * */
int tick_diff(int a, int b) {
  int tmax = (a + tickgap64) % tickmod64;
  if (a > tmax) a = a - tickmod64;
  if (b >= tmax) b = b - tickmod64;
  return a - b;
}

void main()
{
  vec2 pos = fragTexCoord;
  vec2 p2 = vec2(pos.x, 1.0 - pos.y);

  /* Circuit pixel color */
  vec4 c = texture(texture0, p2); 

  /* p is the last EXACT TICK where the circuit has changed */
  int p = unpack_int32(texture(pmap, pos));
  /* NAND or bg */
  if (p == 0 && error_mode == 0) {
    out_circ = vec4(c.rgb * 0.5, 1);
    out_light = vec4(0,0,0,1);
    return;
  }

  /* Encoded value before and after of the last change in the first 4 bits */
  int v0 = p & 3;
  int v1 = (p >> 2)&3;
  /* Actual change tick value */
  p = p >> 4;

  // tick = tick % TICK_MOD;
  /* Relative tick */
  //int rt = tick_diff(64*tick, p);
  int rt = tick_diff(64*tick, p);

  /* Relative time = relative tick + slack_ticks - distmap_tick */
  float f = (float(rt) + 64 * slack);
  f = f / 64.f;
  int v = v0;

  /* f > 0 means the pixel has changed value */
  if (f > 0) {
    v = v1;
  }
  vec4 fc = c;
  vec4 c_on = fc;
  vec4 c_off = c_on;


  // c_off = get_off_color(c_on);
  vec4 cc = c_on;
  get_color_on_off(cc);
  c_on = g_c_on;
  c_off = g_c_off;

  vec4 circ = getcirc(f, v, c_on, c_off);
  vec4 light = getlight(f, c_on, v, v1, c_on, c_off);

  // Set alpha to (1 - ema_factor) for exponential moving average blending

  if (ema_factor > 0) {
    out_circ = circ * ema_factor + (1-ema_factor) * texture(prev_circ, p2);
    out_light = light * ema_factor+ (1-ema_factor) * texture(prev_light, p2);
  } else {
    out_circ = circ; // vec4(circ.rgb, 1.0 - ema_factor);
    out_light = light; //vec4(light.rgb, 1.0 - ema_factor);
  }

}
