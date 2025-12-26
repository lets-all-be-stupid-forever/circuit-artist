#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D comp_x;
uniform sampler2D comp_y;
uniform sampler2D state_buf;

uniform int simu_state;
uniform vec4 bg_color;
uniform vec4 bugged_color;
uniform vec4 undefined_color;
uniform vec2 size_factor;

uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main()
{
    // Texel color fetching from texture sampler
    // vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 src = texture(texture0, fragTexCoord);
    vec4 tx = texture(comp_x, fragTexCoord);
    vec4 ty = texture(comp_y, fragTexCoord);

    vec4 texelColor;
    texelColor = src;
    bool bg = texelColor.a == 0.0;

//     if (texelColor.a == 0.0) {
//         texelColor = vec4(1.0, 0.0, 0.0, 1.0);
//         src = texture(texture0, fragTexCoord + 0.5 * vec2(0, 1) *size_factor);
//         if (src.a > 0.1) {
//            // texelColor = vec4(0.0, 1.0, 0.0, 1.0);
//            src = 0.2 * src;
//            src.a = 1.0;
//            texelColor = src;
//            // src.a = 0.5; 
//         }
//         // if (src.a > 0.1) {
//         //     texelColor = vec4(0.0, 0.0, 0.0, 1.0);
//         //     // src.a = 0.7;
//         // }
//     }

     // Normal wire.
     if (ty.r < 0.9) {
       vec4 ts = texture(state_buf, vec2(tx.r, ty.r));
       texelColor = src;
       float s = ts.r;
       if (s <= 0.05) {
         texelColor = bg_color;
       } else if (s < 0.15) {
         // texelColor = bugged_color;
         texelColor = vec4(1.0, 0.0, 0.0, 1.0);
       } else if (s < 0.25) {
         // undefined = -1
         texelColor = undefined_color;
         if (simu_state == 1) {
            texelColor.a = 0.1;
         }
       } else if (s < 0.35) {
         texelColor.a = 50.0 / 255.0;
       } else if (s < 0.45)  {
         texelColor.a = 1.0;
       }
     } else {
       // Here we have a nand pixel
       if (tx.r < 0.915)  {
          // 0.91
         texelColor = src;
         texelColor.a = 100.0 / 255.0;
       } else if (tx.r < 0.925) {
          // 0.92
         texelColor = undefined_color;
         if (simu_state == 1) {
            texelColor.a = 0.2;
         }
       } else if (tx.r < 0.935) {
         // 0.93
         texelColor = bugged_color;
       }
     }

     if (bg) {
         src = texture(texture0, fragTexCoord + 0.5 * vec2(0, 1) *size_factor);
         if (src.a > 0.1) {
            // texelColor = vec4(0.0, 1.0, 0.0, 1.0);
            // src = 0.2 * src;
            // src.a = 1.0;
            texelColor = 0.4 * texelColor + vec4(0.1, 0.1, 0.1, 0.1);
texelColor.a = 1.0;
            // src.a = 0.5; 
         }
         // if (src.a > 0.1) {
         //     texelColor = vec4(0.0, 0.0, 0.0, 1.0);
         //     // src.a = 0.7;
         // }
     }

    finalColor = texelColor*colDiffuse;
}
