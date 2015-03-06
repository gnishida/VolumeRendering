#version 330
in vec4 glVertex;
out vec4 vPosition; 
out vec4 gl_Position;
uniform mat4 modelviewMatrix;  
uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * modelviewMatrix * glVertex;
    vPosition = glVertex; //object space pos, also texture coord
}