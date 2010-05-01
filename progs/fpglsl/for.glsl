uniform int KernelSizeInt;

void main() {
    int i;
    vec4 sum = vec4(0.0);
    for (i = 0; i < KernelSizeInt; ++i) {
	sum.g += 0.25;
    }
    sum.a = 1.0;
    gl_FragColor = sum;
}
