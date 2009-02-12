// Multi-texture vertex shader
// Brian Paul


attribute vec4 TexCoord0, TexCoord1;
attribute vec4 VertCoord;

void main() 
{
   gl_TexCoord[0] = TexCoord0;
   gl_TexCoord[1] = TexCoord1;
   // note: may use gl_Vertex or VertCoord here for testing:
   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
