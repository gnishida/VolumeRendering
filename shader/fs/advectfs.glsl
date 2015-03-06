#version 330
out vec4 glFragColor;

uniform sampler3D velocity;
uniform sampler3D source;
uniform vec3 inverseSize;
uniform float dt;
uniform float rdx;
uniform float dissipation;

in float zpos;

void main()
{
    vec3 fragCoord = vec3(gl_FragCoord.xy, zpos);
    vec3 u = texture(velocity, inverseSize * fragCoord).xyz;
	vec3 coord = inverseSize * (fragCoord - dt*rdx*u); 

    //GL_LINEAR does trilinear interp, dissipation make it to disappear
    glFragColor = dissipation * texture(source, coord);
}
