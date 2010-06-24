#version 120
/* FAIL - array must have an implicit or explicit size */

void main()
{
  vec4 a[];

  a = vec4 [2] (vec4(1.0), vec4(2.0));

  gl_Position = gl_Vertex;
}
