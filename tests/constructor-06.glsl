#version 120
/* PASS */

uniform mat2 a;

void main()
{
  mat2 b;

  b = mat2(a);

  gl_Position = gl_Vertex;
}
