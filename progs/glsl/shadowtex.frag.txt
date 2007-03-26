// Fragment shader for 2D texture with shadow attenuation
// Brian Paul


uniform sampler2D tex2d;
uniform vec3 lightPos;

void main()
{
   // XXX should compute this from lightPos
   vec2 shadowCenter = vec2(-0.25, -0.25);

   // d = distance from center
   float d = distance(gl_TexCoord[0].xy, shadowCenter);

   // attenuate and clamp
   d = clamp(d * d * d, 0.0, 2.0);

   // modulate texture by d for shadow effect
   gl_FragColor = d * texture2D(tex2d, gl_TexCoord[0].xy, 0.0);
}
