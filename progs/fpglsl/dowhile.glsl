void main() {
    float sum = 0.0;
    do {
	sum += 0.1;
	break;
    } while (true);
    gl_FragColor = vec4(sum);
}
