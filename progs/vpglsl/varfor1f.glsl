const int KernelSize = 16;
uniform float KernelValue1f[KernelSize];

void main(void)
{
    int i;
    vec4 sum = vec4(0.0);
    vec4 tmp = gl_Color;
    gl_Position = gl_Vertex;

    for (i = 0; i < KernelSize; ++i) {
        float x, y, z, w;

        x = KernelValue1f[i]; ++i;
        y = KernelValue1f[i]; ++i;
        z = KernelValue1f[i]; ++i;
        w = KernelValue1f[i];

        sum += tmp * vec4(x, y, z, w);
    }
    gl_FrontColor = sum;
}
