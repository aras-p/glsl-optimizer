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

vec4 myfunc(vec4 x, vec4 mult, vec4 c)
{
   if (x.x >= 0.5) {
      return mult * c;
   } else {
      return mult + c;
   }
}

vec4 func2(vec4 x)
{
    int i;
    vec4 color = vec4(0);
       for (i = 0; i < KernelSize; ++i) {
           vec4 tmp = vec4(1./KernelSize);
           color += myfunc(x, tmp, gl_Color);
       }
    return x * color;
}

vec4 func(vec4 x)
{
    int i;
    vec4 tmp = gl_Color;
    vec4 sum = x;

    for (i = 0; i < KernelSize; ++i) {
        sum = vec4( add_two(sum.x, KernelValue1f[i]) );
    }
    sum = func2(sum);
    return sum;
}

void main(void)
{
    vec4 sum = vec4(0.0);

    sum = func(sum);
    gl_Position = gl_Vertex;
    gl_FrontColor = sum;
}
