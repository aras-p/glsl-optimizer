
void main() {
   gl_Position = gl_Vertex;
   gl_FrontColor = gl_Color;
   if (gl_Position.x < 0.5) {
      gl_FrontColor.y = 1.0;
   }
   gl_FrontColor.xzw = vec3(0, 0, 1);
}
