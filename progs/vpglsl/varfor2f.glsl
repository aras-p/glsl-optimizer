const int KernelSize = 9;
uniform vec2 KernelValue2f[KernelSize];

void main(void)
{
    int i;
    vec4 sum = vec4(0.0);
    vec4 tmp = gl_Color;
    gl_Position = gl_Vertex;

    for (i = 0; i < KernelSize; ++i) {
        vec2 rg, ba;

        rg = KernelValue2f[i];
        ++i;
        if (i < KernelSize)
	    ba = KernelValue2f[i];
	else
	    ba = vec2(0, 0);

        sum += tmp * vec4(rg, ba);
    }
    gl_FrontColor = sum;
}
