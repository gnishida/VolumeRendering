#version 330
in vec4 vPosition;
out vec3 glFragColor[2];

void main()
{
    vec3 pos = vPosition.xyz;

    if (gl_FrontFacing) {
        glFragColor[0] = 0.5 * (pos + 1.0); //store as texture coord
        glFragColor[1] = vec3(0);
    } else {
        glFragColor[0] = vec3(0);
        glFragColor[1] = 0.5 * (pos + 1.0);
    }
}