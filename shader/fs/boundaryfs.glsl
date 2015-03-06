#version 330
out vec3 glFragColor;

uniform sampler3D texture;
uniform float scale; //-1 for velocity, 1 for pressure
uniform int width;
uniform int height;
uniform int depth;

in float zpos;

void main()
{
	ivec3 coord = ivec3(gl_FragCoord.xy, zpos);
	int w = width-1; int h = height-1; int d = depth-1;
	int xoffset = 0; int yoffset = 0; int zoffset = 0;

	//boundary
	if(coord.x == 0 || coord.x == w || coord.y == 0 || coord.y == h || coord.z == 0 || coord.z == d){
		if(coord.x == 0) xoffset = 1;
		if(coord.x == w) xoffset = -1;
		if(coord.y == 0) yoffset = 1;
		if(coord.y == h) yoffset = -1;
		if(coord.z == 0) zoffset = 1;
		if(coord.z == d) zoffset = -1;
		ivec3 newcood = coord +ivec3(xoffset, yoffset, zoffset);
		glFragColor = scale * texelFetch(texture, newcood, 0).xyz;
	}else{
		glFragColor = texelFetch(texture, coord, 0).xyz;
	}
}