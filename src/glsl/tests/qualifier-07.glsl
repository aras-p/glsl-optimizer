/* FAIL - out only allowed in parameter list in GLSL 1.10 */
void main()
{
  out vec4 foo;

  gl_Position = gl_Vertex;
}
