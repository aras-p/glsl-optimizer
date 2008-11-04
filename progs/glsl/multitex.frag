// Multi-texture fragment shader
// Brian Paul

// Composite second texture over first.
// We're assuming the 2nd texture has a meaningful alpha channel.

uniform sampler2D tex1;
uniform sampler2D tex2;

vec4 sample(sampler2D t, vec2 coord)
{
   return texture2D(t, coord);
}

void main0()
{
   vec4 t1 = texture2D(tex1, gl_TexCoord[0].xy);
   //vec4 t1 = sample(tex1, gl_TexCoord[0].xy);
   //vec4 t2 = texture2D(tex2, gl_TexCoord[1].xy);
   vec4 t2 = sample(tex2, gl_TexCoord[0].xy);
   gl_FragColor = mix(t1, t2, t2.w);
}

void main()
{
   vec4 t1 = sample(tex1, gl_TexCoord[0].xy);
   vec4 t2 = sample(tex2, gl_TexCoord[0].xy);
   gl_FragColor = t1 + t2;
}
/*
  0: MOV SAMPLER[0].x, SAMPLER[0];
  1: MOV TEMP[1], INPUT[4];
  2: TEX OUTPUT[0], TEMP[1], texture[0], 2D;
  3: END
*/
