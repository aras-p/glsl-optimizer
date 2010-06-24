/* FAIL - type of second two operands must match */

uniform bool a;

void main()
{
  gl_Position = (a) ? vec4(1.0, 0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
}
