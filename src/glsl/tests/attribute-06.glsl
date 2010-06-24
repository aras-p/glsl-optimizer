/* FAIL - attribute cannot have type bvec2 */
attribute bvec2 i;

void main()
{
  gl_Position = vec4(1.0);
}
