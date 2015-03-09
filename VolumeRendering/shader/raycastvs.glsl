#version 330
in vec4 glVertex;
out vec3 vPosition; 
out vec4 gl_Position;

uniform mat4 modelviewMatrix;  
uniform mat4 projectionMatrix;


void main()
{
    gl_Position = projectionMatrix * modelviewMatrix * glVertex;
    vPosition = glVertex.xyz; //object space pos, also texture coord
}
