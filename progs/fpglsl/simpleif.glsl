void main() {
    // this should always be true
    if (gl_FragCoord.x >= 0.0) {
	gl_FragColor = vec4(0.5, 0.0, 0.5, 1.0);
    }
}
