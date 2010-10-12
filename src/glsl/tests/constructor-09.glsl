/* PASS */

uniform int a;
uniform float b;
uniform bool c;

void main()
{
  float x;
  int y;
  bool z;

  x = float(a);
  x = float(b);
  x = float(c);

  y = int(a);
  y = int(b);
  y = int(c);

  z = bool(a);
  z = bool(b);
  z = bool(c);

  gl_Position = gl_Vertex;
}
