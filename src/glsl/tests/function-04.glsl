/* FAIL - type mismatch in assignment */

vec3 foo(float x, float y, float z)
{
  vec3 v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

void main()
{
  gl_Position = foo(1.0, 1.0, 1.0);
}
