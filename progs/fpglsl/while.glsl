void main() {
    float sum = 0.0;
    while (sum < 0.499999) {
	sum += 0.1;
    }
    gl_FragColor = vec4(sum);
}
