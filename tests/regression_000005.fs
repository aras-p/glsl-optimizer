uniform vec4 foo;
void main(void)
{
    gl_FragColor.xw = vec2(1.0, 2.0);
    gl_FragColor.yz = 2 * foo.xw;
}