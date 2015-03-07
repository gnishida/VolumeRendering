#version 330
out vec4 glFragColor;

uniform sampler3D x;
uniform sampler3D b;
uniform float alpha;
uniform float rBeta;

in float zpos;

void main()
{
    ivec3 coords = ivec3(gl_FragCoord.xy, zpos);

    // Find neighboring x:
    vec4 xR = texelFetchOffset(x, coords, 0, ivec3(1, 0, 0));
    vec4 xL = texelFetchOffset(x, coords, 0, ivec3(-1, 0, 0));
    vec4 xT = texelFetchOffset(x, coords, 0, ivec3(0, 1, 0));
    vec4 xB = texelFetchOffset(x, coords, 0, ivec3(0, -1, 0));
    vec4 xU = texelFetchOffset(x, coords, 0, ivec3(0, 0, 1));
    vec4 xD = texelFetchOffset(x, coords, 0, ivec3(0, 0, -1));

    vec4 bC = texelFetch(b, coords, 0);

    glFragColor = (xL + xR + xB + xT + xU + xD + alpha * bC) * rBeta;
}