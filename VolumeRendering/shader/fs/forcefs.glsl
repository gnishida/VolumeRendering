#version 330
out vec4 glFragColor;

uniform vec3 pos;
uniform float radius;
uniform vec3 force;

in float zpos;

void main()
{
    float d = distance(pos, vec3(gl_FragCoord.xy, zpos));
    if (d < radius) {
		vec3 newforce = force*d/radius;
		glFragColor = vec4(newforce, 1.0); 
    } else {
        glFragColor = vec4(0);
    }
}