uniform sampler2D tex1;

void main()
{
   gl_FragColor = texture2D(tex1, gl_Color.xy);
}
