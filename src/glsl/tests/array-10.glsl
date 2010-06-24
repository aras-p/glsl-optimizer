/* FAIL - array constructors forbidden in GLSL 1.10
 *
 * This can also generate an error because the 'vec4[]' style syntax is
 * illegal in GLSL 1.10.
 */
void main()
{
  vec4 a[2] = vec4 [2] (vec4(1.0), vec4(2.0));

  gl_Position = gl_Vertex;
}
