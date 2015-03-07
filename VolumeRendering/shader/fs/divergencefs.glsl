#version 330
out float glFragColor;

uniform sampler3D velocity;
uniform float halfrdx;

in float zpos;

void main()
{
    ivec3 coord = ivec3(gl_FragCoord.xy, zpos);

    // Find neighboring velocities:
	vec3 vR = texelFetchOffset(velocity, coord, 0, ivec3(1, 0, 0)).xyz;
    vec3 vL = texelFetchOffset(velocity, coord, 0, ivec3(-1, 0, 0)).xyz;
    vec3 vT = texelFetchOffset(velocity, coord, 0, ivec3(0, 1, 0)).xyz;
    vec3 vB = texelFetchOffset(velocity, coord, 0, ivec3(0, -1, 0)).xyz;
    vec3 vU = texelFetchOffset(velocity, coord, 0, ivec3(0, 0, 1)).xyz;
    vec3 vD = texelFetchOffset(velocity, coord, 0, ivec3(0, 0, -1)).xyz;

    glFragColor = halfrdx * (vR.x - vL.x + vT.y - vB.y + vU.z - vD.z);
}