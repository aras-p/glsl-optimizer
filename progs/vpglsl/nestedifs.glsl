
void main() {
   gl_Position = gl_Vertex;
    if (gl_Position.x < 0.5) {
	if (gl_Position.y < 0.20) {
	    gl_FrontColor = vec4(1.0, 0.0, 0.0, 1.0);
	} else {
	    gl_FrontColor = vec4(0.0, 1.0, 0.0, 1.0);
	}
        gl_FrontColor.y = 1.0;
    } else
	gl_FrontColor = gl_Color;
}
