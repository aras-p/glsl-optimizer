uniform vec4 foo;

void main()
{
gl_FragColor.r = foo.r > 0.25f ? 1.0f : 0.5f;
}