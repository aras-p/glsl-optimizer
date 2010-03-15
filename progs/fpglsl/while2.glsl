void main() {
    float sum = 0.0;
    while (true) {
	sum += 0.1;
        if (sum > 0.8)
           break;
    }
    gl_FragColor = vec4(sum);
}
