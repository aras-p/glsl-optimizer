#version 120
const int KernelSize = 16;
uniform float KernelValue1f[KernelSize];


float add_two(float a, float b)
{
    if (a > b)
        return a - b;
    else
        return a + b;
}

vec4 func(vec4 x)
{
    int i;
    vec4 tmp = gl_Color;
    vec4 sum = x;

    for (i = 0; i < KernelSize; ++i) {
        sum = vec4( add_two(sum.x, KernelValue1f[i]) );
    }
    return sum;
}

void main(void)
{
    vec4 sum = vec4(0.0);

    sum = func(sum);
    gl_Position = gl_Vertex;
    gl_FrontColor = sum;
}
