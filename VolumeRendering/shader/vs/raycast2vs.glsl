#version 330
in vec4 glVertex;
out vec2 vPosition;

void main()
{
    gl_Position = glVertex;
    vPosition = glVertex.xy;
}
