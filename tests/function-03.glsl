/* PASS */

vec4 foo(in float x, in float y, float z, float w)
{
  vec4 v;
  v.x = x;
  v.y = y;
  v.z = z;
  v.w = w;
  return v;
}

void main()
{
  gl_Position = foo(1.0, 1.0, 1.0, 0.0);
}
