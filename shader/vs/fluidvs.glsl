#version 330
in vec4 glVertex;
out int vInstanceID;

void main()
{
    gl_Position = glVertex;
    vInstanceID = gl_InstanceID; //depth instance of same points
}