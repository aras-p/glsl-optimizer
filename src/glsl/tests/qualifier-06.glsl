/* FAIL - in only allowed in parameter list in GLSL 1.10 */
void main()
{
  in vec4 foo;

  gl_Position = gl_Vertex;
}
