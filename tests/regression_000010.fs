varying vec3 foo;
varying vec3 bar;
void main(void)
{
    gl_FragColor.r = dot(foo.zyx, bar);
}