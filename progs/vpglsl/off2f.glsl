const int KernelSize = 8;
uniform vec2 Offset2f[KernelSize];
uniform vec4 KernelValue4f[KernelSize];

void main(void)
{
    int i;
    vec4 sum = vec4(0.0);
    vec4 tmp = gl_Color;
    vec2 rg, ba;
    gl_Position = gl_Vertex;

    rg = Offset2f[4];
    ba = Offset2f[5];


    gl_FrontColor = KernelValue4f[0] * vec4(rg, ba);
}
