// Vertex shader for cube-texture reflection mapping
// Brian Paul


varying vec3 normal;

void main() 
{
   vec3 n = gl_NormalMatrix * gl_Normal;
   vec3 u = normalize(vec3(gl_ModelViewMatrix * gl_Vertex));
   float two_n_dot_u = 2.0 * dot(n, u);
   vec4 f;
   f.xyz = u - n * two_n_dot_u;

   // outputs
   normal = n;
   gl_TexCoord[0] = gl_TextureMatrix[0] * f;
   gl_Position = ftransform();
}
