/* PASS */

uniform bvec4 a;

void main()
{
  if (a.x)
    gl_Position = vec4(1.0, 0.0, 0.0, 1.0);
  else
    gl_Position = vec4(0.0, 1.0, 0.0, 1.0);
}
