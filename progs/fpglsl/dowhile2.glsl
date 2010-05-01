void main() {
    float sum = 0.0;
    do {
	sum += 0.1;
	if (sum < 0.499999)
	    continue;
	break;
    } while (true);
    gl_FragColor = vec4(sum);
}
