// Vertex weighting/blendin shader
// Brian Paul
// 4 Nov 2008

uniform mat4 mat0, mat1;
attribute float weight;

void main() 
{
   // simple diffuse shading
   // Note that we should really transform the normal vector along with
   // the postion below... someday.
   vec3 lightVec = vec3(0, 0, 1);
   vec3 norm = gl_NormalMatrix * gl_Normal;
   float dot = 0.2 + max(0.0, dot(norm, lightVec));
   gl_FrontColor = vec4(dot);

   // compute sum of weighted transformations
   vec4 pos0 = mat0 * gl_Vertex;
   vec4 pos1 = mat1 * gl_Vertex;
   vec4 pos = mix(pos0, pos1, weight);

   gl_Position = gl_ModelViewProjectionMatrix * pos;
}
