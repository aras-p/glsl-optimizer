/* PASS */

uniform float a;
uniform float b;

void main()
{
  ivec2 c;

  c = ivec2(a, b);

  gl_Position = gl_Vertex;
}
