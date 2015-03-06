#version 330
out vec3 glFragColor;

uniform sampler3D velocity;
uniform sampler3D temperature;
uniform sampler3D density;
uniform float ambientTemperature;
uniform float dt;
uniform float sigma;
uniform float kappa;
uniform float material;
uniform vec3 dir;

in float zpos;

void main()
{
    ivec3 coord = ivec3(gl_FragCoord.xy, zpos);
    float T = texelFetch(temperature, coord, 0).x;
    vec3 V = texelFetch(velocity, coord, 0).xyz;

    glFragColor = V;

    if (T > ambientTemperature) {
		float d = texelFetch(density, coord, 0).x;
		glFragColor += (dt * (T - ambientTemperature) * sigma - d * kappa )* dir; 
    }
}
