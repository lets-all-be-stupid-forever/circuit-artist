#version 330
out vec4 finalColor;
// uniform sampler2D dmap;

in vec2 fragTexCoord;
in vec4 pvalue;
flat in vec2 v_dist;
flat in vec2 flat_p;
flat in float rc_seg;
in float v_l;
uniform int tickmod64;

float unpack_float32(vec4 rgba) {
  uvec4 bytes = uvec4(rgba * 255.0 + 0.5);
  uint uval = (bytes.a << 24) | (bytes.b << 16) |
              (bytes.g << 8) | bytes.r;
  return uintBitsToFloat(uval);
}

#if 1
// p0 --> first pixel
// p1 --> last pixel
// p --> cur pixel
// d0 --> first distance
// d1 --> last distance
// Do I need to pass p0,p1,p? Or can i just pass p ?
float interp(int p0, int p1, int p, float rc, float d0,
                           float d1) {
  if (d0 == 0 && d1 == 0) {
    return 0.f;
  }
  float a, t0, t1;
  if (p0 == p) return d0;
  if (p1 == p) return d1;
  float fp0 = p0;
  float fp1 = p1;
  float fp = p;
  if (d0 < d1) {
    a = (fp - fp0) / (fp1 - fp0);
    t0 = d0;
    t1 = d1;
  } else {
    a = (fp1 - fp) / (fp1 - fp0);
    t0 = d1;
    t1 = d0;
  }
  return t0 + (t1 - t0) * a + rc * a * (1.f - a);
}
#endif


int unpack_int32(vec4 rgba) {
  uvec4 bytes = uvec4(rgba * 255.0 + 0.5);  // Denormalize
  uint uval = (bytes.a << 24) | (bytes.b << 16) |
    (bytes.g << 8) | bytes.r;      // Reassemble bytes
  return int(uval);
}

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
  /* I only update if dmap is the same orientation */
  // float d = unpack_float32(texture(dmap, fragTexCoord));

  int p0 = int(floor(flat_p.x + 0.5));
  int p1 = int(floor(flat_p.y + 0.5));
  int p = int(floor(v_l * (flat_p.y - flat_p.x) + flat_p.x + 0.5));
  float d0 = v_dist.x;
  float d1 = v_dist.y;

  // Maybe I should transform the pulse to get tick + dist and the v0,v1
  float d = interp(p0, p1, p, rc_seg, d0, d1);

  // d = d / 20.0;

  int pp = unpack_int32(pvalue);
  int v0 = pp & 3;
  int v1 = (pp >> 2)&3;
  int p2 = pp >> 4;

  int dist_i = int(64 * d);
  // unscaled
  p2 = p2 * 64;

  int k = dist_i + p2;
  k = k % tickmod64;

  // Compress again:

  int new_p = (pp & (15)) | (k << 4);

  vec4 packed_p =  pack_int32(new_p);

  // packed_p.a=1.0;
  // d = (p2 / 20.0 + d) / 50.0;
  // float d = (v_dist.x  + v_dist.y)/ 100.0;
  // finalColor = vec4(d,d,d,1.0);

  finalColor = packed_p;
  // finalColor = pvalue;

// ori = 1 ==> Vertical
// ori = 0 ==> Horizontal
//  if ((ori > 0.5 && d <= 0) || (ori < 0.5 && d >= 0)) {
//    finalColor = pvalue;
//  } else {
//    discard;
//  }


//   finalColor = vec4(1,1,1,1.0);

//   if ((ori > 0.5 && d <= 0) || (ori < 0.5 && d >= 0)) {
//     finalColor = pvalue;
//   } else {
// discard;
// }
    // finalColor = vec4(0);
  //finalColor = vec4(0,1,1,0.5);
}
