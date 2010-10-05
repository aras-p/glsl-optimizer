#version 120
/* PASS */

void main()
{
  vec4 a[2] = vec4 [2] (vec4(1.0), vec4(2.0));

  gl_Position = gl_Vertex;
}
