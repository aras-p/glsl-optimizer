/* FAIL - too few components supplied to constructor */

uniform vec2 a;
uniform float x;

void main()
{
  mat2 b;

  b = mat2(a, x);

  gl_Position = gl_Vertex;
}
