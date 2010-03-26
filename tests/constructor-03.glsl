/* FAIL - cannot construct a matrix from a matrix in GLSL 1.10 */

uniform mat2 a;

void main()
{
  mat2 b;

  b = mat2(a);

  gl_Position = gl_Vertex;
}
