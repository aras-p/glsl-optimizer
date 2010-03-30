/* FAIL - attribute cannot have type ivec2 */
attribute ivec2 i;

void main()
{
  gl_Position = vec4(1.0);
}
