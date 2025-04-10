#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform int ccw;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main()
{
    vec2 coord2;
    if (ccw == 0) {
        coord2 = vec2(-fragTexCoord.y, fragTexCoord.x);
    } else {
        coord2 = vec2(fragTexCoord.y, -fragTexCoord.x);
    }
    vec4 texelColor = texture(texture0, coord2);
    finalColor = texelColor*colDiffuse;
}
