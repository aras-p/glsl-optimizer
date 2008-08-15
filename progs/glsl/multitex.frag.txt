// Multi-texture fragment shader
// Brian Paul

// Composite second texture over first.
// We're assuming the 2nd texture has a meaningful alpha channel.

uniform sampler2D tex1;
uniform sampler2D tex2;

void main()
{
   vec4 t1 = texture2D(tex1, gl_TexCoord[0].xy);
   vec4 t2 = texture2D(tex2, gl_TexCoord[1].xy);
   gl_FragColor = mix(t1, t2, t2.w);
}
