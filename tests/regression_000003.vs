varying vec2 TexCoords;
void main()
{
    TexCoords = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}