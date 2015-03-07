#version 330
out vec3 glFragColor;

uniform sampler3D velocity;
uniform sampler3D pressure;
uniform float halfrdx;

in float zpos;

void main()
{
    ivec3 coord = ivec3(gl_FragCoord.xy, zpos);

    // Find neighboring pressure:
    float pR = texelFetchOffset(pressure, coord, 0, ivec3(1, 0, 0)).x;
    float pL = texelFetchOffset(pressure, coord, 0, ivec3(-1, 0, 0)).x;
    float pT = texelFetchOffset(pressure, coord, 0, ivec3(0, 1, 0)).x;
    float pB = texelFetchOffset(pressure, coord, 0, ivec3(0, -1, 0)).x;
    float pU = texelFetchOffset(pressure, coord, 0, ivec3(0, 0, 1)).x;
    float pD = texelFetchOffset(pressure, coord, 0, ivec3(0, 0, -1)).x;

    vec3 V = texelFetch(velocity, coord, 0).xyz;
    vec3 newV = V - vec3(pR - pL, pT - pB, pU - pD) * halfrdx;

	glFragColor = newV;
}