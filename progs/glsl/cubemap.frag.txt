// Fragment shader for cube-texture reflection mapping
// Brian Paul


uniform samplerCube cubeTex;
varying vec3 normal;
uniform vec3 lightPos;

void main()
{
   // simple diffuse, specular lighting:
   vec3 lp = normalize(lightPos);
   float dp = dot(lp, normalize(normal));
   float spec = pow(dp, 5.0);

   // final color:
   gl_FragColor = dp * textureCube(cubeTex, gl_TexCoord[0].xyz, 0.0) + spec;
}
