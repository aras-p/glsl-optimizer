
void main() {
    gl_FrontColor = gl_Color;
    gl_PointSize = 10 * gl_Color.x;
    gl_Position = gl_Vertex;
}
