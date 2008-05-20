
const int KernelSize = 9;

//texture offsets
uniform vec2 Offset[KernelSize];
//convolution kernel
uniform vec4 KernelValue[KernelSize];
uniform sampler2D srcTex;
uniform vec4 ScaleFactor;
uniform vec4 BaseColor;

void main(void)
{
    int i;
    vec4 sum = vec4(0.0);
    for (i = 0; i < KernelSize; ++i) {
        vec4 tmp = texture2D(srcTex, gl_TexCoord[0].st + Offset[i]);
        sum += tmp * KernelValue[i];
    }
    gl_FragColor = sum * ScaleFactor + BaseColor;
}
