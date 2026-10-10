#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

/* Blits src over dst. The caller sets blend factors to (ONE, ZERO), so whatever
 * is written here replaces the destination pixel outright.
 *
 * This used to take a dst_tex sampler and write the destination value back for
 * pixels it wanted to leave alone -- which made the shader read the very
 * framebuffer it renders into. Desktop GL leaves that undefined (and in
 * practice returns the current contents, so it appeared to work); WebGL treats
 * it as a feedback loop, raises INVALID_OPERATION and drops the whole draw
 * call, so nothing ever landed in the destination texture. discard leaves the
 * pixel untouched without reading it, and needs no dst sampler at all. */
void main()
{
    vec4 src = texture(texture0, fragTexCoord);
    if (src.r == 0.0 && src.g == 0.0 && src.b == 0.0 && src.a == 1.0) {
        /* Pure opaque black is the "erase to background" sentinel. */
        finalColor = vec4(0.0, 0.0, 0.0, 0.0);
    } else if (src.a != 0.0) {
        finalColor = src * colDiffuse;
    } else {
        discard;
    }
}
