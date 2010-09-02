/* PASS */

uniform bool a;

void main()
{
  gl_Position = (a) ? vec4(1.0, 0.0, 0.0, 1.0) : vec4(0.0, 1.0, 0.0, 1.0);
}
