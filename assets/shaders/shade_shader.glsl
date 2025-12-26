#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 size_factor; 

// Output fragment color
out vec4 finalColor;

void main()
{
    // Texel color fetching from texture sampler
    // NOTE: Implement here your fragment shader code

    const int range = 6; // should be = (samples - 1)/2;
    vec4 sum = vec4(0);
    float nbg = 0.0;
    float nc = 0.0;
    float tot = 0.0;
    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            vec4 s = texture(texture0, fragTexCoord + 0.25 * vec2(x, y)*size_factor);
            if (s.a > 0.5) {
              nc = nc + 1.0;
            } else {
              nbg = nbg + 1.0;
            }
            tot = tot + 1.0;
        }
    }

    vec4 texelColor;
    texelColor.r = 0.0;
    texelColor.g = 0.0;
    texelColor.b = 0.0;
    texelColor.a = nc / tot;
    // if (nc < 1.0) {
    //     texelColor.a = 0.2;
    // }

    // float k = 2.0 * range + 1.0;
    // sum = sum / (k * k);
    finalColor = texelColor*colDiffuse *fragColor;
}
