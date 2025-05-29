#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;
uniform vec2 size_factor; // 1 screen pixel delta in texture coords. Nao sei se suficiente..

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

    // NOTE: Implement here your fragment shader code

    const int range = 1; // should be = (samples - 1)/2;
    vec4 sum = vec4(0);
    int nbg = 0;
    int nc = 0;
    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            vec4 s = texture(texture0, fragTexCoord + vec2(x, y)*size_factor);
            if (s.a > 0.5) {
              nc = nc + 1;
            } else {
              nbg = nbg + 1;
            }
        }
    }

    texelColor.r = 1.0;
    texelColor.g = 1.0;
    texelColor.b = 1.0;
    if (nc == 0 ) {
        texelColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
    if (nbg == 0) {
        texelColor = vec4(0.0, 0.0, 0.0, 0.0);
    }

    // float k = 2.0 * range + 1.0;
    // sum = sum / (k * k);
    finalColor = texelColor*colDiffuse *fragColor;
}
