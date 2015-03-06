#version 330
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
 
in int vInstanceID[3];//instance ID for vertices on triangle, depth num instances
out float zpos;
 
void main()
{
    gl_Layer = vInstanceID[0];
    zpos = float(gl_Layer) + 0.5;//get the center point
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
}