/* FAIL - attribute cannot have type ivec4 */
attribute ivec4 i;

void main()
{
  gl_Position = vec4(1.0);
}
