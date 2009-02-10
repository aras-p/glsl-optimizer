
void main() {
   gl_Position = gl_Vertex;
   gl_FrontColor = vec4(0);
   for (int i = 0; i < 4; ++i)
       gl_FrontColor += gl_Color;
}
