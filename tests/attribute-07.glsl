/* FAIL - attribute cannot have type bvec3 */
attribute bvec3 i;

void main()
{
  gl_Position = vec4(1.0);
}
