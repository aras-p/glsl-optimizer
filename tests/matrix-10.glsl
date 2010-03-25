#version 120
/* PASS */

uniform mat3 a;

void main()
{
    mat2 m;

    m = mat2(a);
    gl_Position = gl_Vertex;
}
