const int KernelSize = 4;
uniform vec4 KernelValue4f[KernelSize];

void main(void)
{
    int i;
    vec4 sum = vec4(0.0);
    vec4 tmp = gl_Color;
    gl_Position = gl_Vertex;

    for (i = 0; i < KernelSize; ++i) {
        vec4 rgba;

        rgba = KernelValue4f[i];

        sum += tmp * rgba;
    }
    gl_FrontColor = sum;
}
